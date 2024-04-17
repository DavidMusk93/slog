#include "file.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
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

std::shared_ptr<File> File::of(const char* path, Buf buf) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
            r.push_back(std::string{
                s.substr(name_.size(), s.size() - name_.size() - ext_.size())});
            return 0;
        },
        [](auto p) {
            std::cerr << internal::format("opendir {} failed: {}"sv, p, strerror(errno))
                      << std::endl;
        });

    return r;
}

std::string RotatePolicy::Path(std::string_view tag) const {
    std::ostringstream ss;
    ss << base_ << '/' << name_ << tag << ext_;
    return ss.str();
}

RotatePolicy RotatePolicy::Builder::Build() {
    return {std::move(base_), std::move(name_), std::move(ext_), n_, buf_size_};
}

SizeRotate::SizeRotate(RotatePolicy policy, uint64_t size)
    : policy_{std::move(policy)}, size_{size}, written_{0}, i_{0} {
    witness_.reserve(policy_.max_files());
    const int n = policy_.max_files();
    for (int i = 0; i < n; ++i) {
        witness_.push_back(policy_.Path(std::to_string(i)));
    }
    buf_.resize(policy_.buf_size());

    std::string latest;
    time_t latest_ts{};

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
        struct stat st {};
        stat(f.c_str(), &st);
        if (latest_ts < st.st_atim.tv_sec) {
            latest = std::move(s);
            latest_ts = st.st_atim.tv_sec;
            i_ = i;
        }
    }
}

std::shared_ptr<File> SizeRotate::Next() {
    written_ = 0;
    if (i_ == policy_.max_files()) i_ = 0;
    unlink(policy_.slink().c_str());
    symlink(witness_[i_].c_str(), policy_.slink().c_str());
    return File::of(witness_[i_++].c_str(), Buf::of(buf_));
}

bool SizeRotate::Spill(Metadata metadata) {
    if (written_ + metadata.size >= size_) {
        return true;
    }
    written_ += metadata.size;
    return false;
}

std::shared_ptr<SizeRotate> SizeRotate::Builder::Build() {
    return std::make_shared<trampoline<SizeRotate>>(
        static_cast<RotatePolicy::Builder*>(this)->Build(), size_);
}

}  // namespace slog