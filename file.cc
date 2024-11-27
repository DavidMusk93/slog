#include "file.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include "iter.h"
#include "str.h"

namespace slog {

Buf::Buf(char* a, int n) : a_{a}, i_{0}, n_{n} {}

bool Buf::Write(std::string_view s) {
    const int n = s.size();
    if (i_ + n > n_) return false;

    memcpy(a_ + i_, s.data(), n);
    i_ += n;
    return true;
}

std::string_view Buf::Rewind() {
    std::string_view r{a_, (size_t)i_};
    i_ = 0;
    return r;
}

Buf Buf::of(char* a, int n) {
    return {a, n};
}

Buf Buf::of(std::string& s) {
    return {s.data(), (int)s.size()};
}

File::File(int fd, Buf buf) : fd_{fd}, buf_{buf} {}

File::~File() {
    Flush();
    if (fd_ != -1) close(fd_);
}

#define _write(_s)                                                \
    do {                                                          \
        if (_s.empty()) break;                                    \
        if (write(fd_, _s.data(), _s.size()) == -1) return false; \
    } while (false)

bool File::Write(std::string_view s) {
    if (buf_.Write(s)) return true;

    auto r = buf_.Rewind();
    _write(r);
    _write(s);
    return true;
}

bool File::Flush() {
    auto r = buf_.Rewind();
    _write(r);

    return fdatasync(fd_) == 0;
}

std::shared_ptr<File> File::of(const char* path, Buf buf, bool append) {
    int fd = open(path, O_WRONLY | O_CREAT | std::array{O_TRUNC, O_APPEND}[append], 0644);
    if (fd < 0) {
        std::cerr << internal::format("fallback to stdout: open {} failed: {}"sv, path,
                                      strerror(errno))
                  << std::endl;
        fd = dup(STDOUT_FILENO);
    }

    return of(fd, buf);
}

std::shared_ptr<File> File::of(int fd, Buf buf) {
    return std::make_shared<trampoline<File>>(fd, buf);
}

auto RotatePolicy::Builder::set_buf_size(Bytes bytes) -> Builder& {
    buf_size_ = bytes.value();
    return *this;
}

RotatePolicy::RotatePolicy(std::string base, std::string name, std::string ext, int n,
                           int buf_size)
    : base_{std::move(base)},
      name_{std::move(name)},
      ext_{std::move(ext)},
      n_{n},
      buf_size_{buf_size} {
    slink_ = Path(""s);
}

std::vector<std::string> RotatePolicy::Probe() const {
    decltype(Probe()) r;
    r.reserve(n_);
    unlink(slink_.c_str());

    internal::iter(
        base_,
        [&](auto e) -> int {
            if (e->d_type != DT_REG) return 0;
            std::string_view s{e->d_name};
            if (!internal::starts_with(s, name_)) return 0;
            if (!internal::ends_with(s, ext_)) return 0;
            s.remove_prefix(name_.size() + 1);
            s.remove_suffix(ext_.size() + 1);
            r.push_back(std::string{s});
            return 0;
        },
        [](auto& p) {
            std::cerr << internal::format("opendir {} failed: {}"sv, p, strerror(errno))
                      << std::endl;
        });

    return r;
}

std::string RotatePolicy::Path(std::string_view tag) const {
    std::ostringstream ss;
    ss << base_ << '/' << name_;
    if (!tag.empty()) ss << '.' << tag;
    ss << '.' << ext_;
    return ss.str();
}

RotatePolicy RotatePolicy::Builder::Build() {
    return {std::move(base_), std::move(name_), std::move(ext_), n_, buf_size_};
}

static bool operator<(struct timespec lhs, struct timespec rhs) {
    return lhs.tv_sec < rhs.tv_sec ||
           (lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec < rhs.tv_nsec);
}

class latest_file {
    std::string name_;
    struct timespec ts_ {};
    size_t size_{};

   public:
    bool probe(std::string f) {
        struct stat st {};
        stat(f.c_str(), &st);
        if (ts_ < st.st_mtim) {
            ts_ = st.st_mtim;
            name_ = std::move(f);
            size_ = st.st_size;
            return true;
        }
        return false;
    }
    auto size() const {
        return size_;
    }
    bool trivial() const {
        return name_.empty();
    }
};

auto SizeRotate::Builder::set_size(Bytes bytes) -> Builder& {
    size_ = bytes.value();
    return *this;
}

SizeRotate::SizeRotate(RotatePolicy policy, uint64_t size)
    : policy_{std::move(policy)}, size_{size}, written_{0}, i_{0} {
    witness_.reserve(policy_.max_files());
    const int n = policy_.max_files();
    for (int i = 0; i < n; ++i) {
        witness_.push_back(policy_.Path(std::to_string(i)));
    }
    buf_.resize(policy_.buf_size());

    latest_file latest;
    auto v = policy_.Probe();

    for (auto& s : v) {
        char* e{};
        int i = strtol(s.c_str(), &e, 10);
        auto f = policy_.Path(s);
        if (*e || i >= n) {
            // remove unrelated files
            unlink(f.c_str());
            continue;
        }
        if (latest.probe(policy_.Path(s))) i_ = i;
    }
    if (!latest.trivial()) latest_ = std::make_shared<latest_file>(std::move(latest));
}

std::shared_ptr<File> SizeRotate::Next() {
    written_ = 0;
    if (i_ == policy_.max_files()) i_ = 0;
    unlink(policy_.slink().c_str());
    auto& f = witness_[i_];
    symlink(f.substr(f.rfind('/') + 1).c_str(), policy_.slink().c_str());

    bool append = false;
    if (latest_) {
        if (latest_->size() < size_) {
            append = true;
            written_ = latest_->size();
        }
        latest_.reset();
    }
    return File::of(witness_[i_++].c_str(), Buf::of(buf_), append);
}

bool SizeRotate::Spill(Metadata metadata) {
    if (written_ + metadata.size >= size_) return true;
    written_ += metadata.size;
    return false;
}

std::shared_ptr<SizeRotate> SizeRotate::Builder::Build() {
    return std::make_shared<trampoline<SizeRotate>>(
        static_cast<RotatePolicy::Builder*>(this)->Build(), size_);
}

auto TimeRotate::Builder::set_span(Seconds seconds) -> Builder& {
    static const std::array a{"1h"_s, "2h"_s, "3h"_s, "4h"_s, "6h"_s, "8h"_s, "12h"_s,
                              "1d"_s, "2d"_s, "3d"_s, "4d"_s, "6d"_s, "7d"_s};
    span_ = seconds.value();
    // align span
    if (auto it = std::lower_bound(
            a.begin(), a.end(), span_,
            [](auto element, auto value) { return element.value() < value; });
        it != a.end()) {
        span_ = it->value();
    } else {
        span_ = a.back().value();
    }
    return *this;
}

std::shared_ptr<TimeRotate> TimeRotate::Builder::Build() {
    return std::make_shared<trampoline<TimeRotate>>(
        static_cast<RotatePolicy::Builder*>(this)->Build(), span_);
}

TimeRotate::TimeRotate(RotatePolicy policy, int64_t span)
    : policy_{std::move(policy)}, span_{span} {
    timeval tv{};
    tm tm{};
    const auto day_seconds = "1d"_s.value();
    gettimeofday(&tv, nullptr);
    localtime_r(&tv.tv_sec, &tm);
    tm.tm_min = 0;
    tm.tm_sec = 0;
    if (span_ >= day_seconds) {
        current_time_ = tv.tv_sec - tv.tv_sec % day_seconds;
    } else {
        current_time_ = tv.tv_sec - tv.tv_sec % span_;
    }
    auto earliest_time = current_time_ - policy_.max_files() * span_;

    buf_.resize(policy_.buf_size());
    auto v = policy_.Probe();

    std::vector<std::pair<int, int>> rest;
    int i{};
    rest.reserve(policy_.max_files());

    for (auto& s : v) {
        ++i;
        int n = sscanf(s.c_str(), "%d-%d-%d_%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                       &tm.tm_hour);
        if (n != 4) {
            // remove unrelated files
            unlink(policy_.Path(s).c_str());
            continue;
        }
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        auto t = mktime(&tm);
        if (t <= earliest_time) {
            // remove early files
            unlink(policy_.Path(s).c_str());
            continue;
        }
        rest.push_back({i - 1, (int)t});
    }

    std::sort(rest.begin(), rest.end(),
              [](auto lhs, auto rhs) { return lhs.second < rhs.second; });
    if (auto it = std::lower_bound(
            rest.begin(), rest.end(), current_time_,
            [](auto element, auto value) { return element.second < value; });
        it != rest.end()) {
        rest.resize(it - rest.begin());
    }

    i = 0;
    for (auto it = rest.rbegin(); it != rest.rend(); ++it) {
        if (++i == policy_.max_files()) break;
        witness_.push_front(policy_.Path(v[it->first]));
    }
}

std::string TimeRotate::new_file() const {
    char buf[16];
    tm tm{};
    localtime_r(&current_time_, &tm);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d_%02d", tm.tm_year + 1900, tm.tm_mon + 1,
             tm.tm_mday, tm.tm_hour);
    return policy_.Path(std::string{buf});
}

std::shared_ptr<File> TimeRotate::Next() {
    if ((int)witness_.size() >= policy_.max_files()) {
        auto t = std::move(witness_.front());
        witness_.pop_front();
        unlink(t.c_str());
    }

    witness_.push_back(new_file());

    unlink(policy_.slink().c_str());
    symlink(witness_.back().c_str(), policy_.slink().c_str());

    return File::of(witness_.back().c_str(), Buf::of(buf_), true);
}

bool TimeRotate::Spill(Metadata metadata) {
    if (metadata.seconds <= current_time_ + span_) return false;
    current_time_ += span_;
    return true;
}

}  // namespace slog
