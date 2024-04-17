#pragma once

#include <memory>
#include <string>
#include <vector>

namespace slog {

using namespace std::literals;

template <typename T>
class trampoline : public T {
   public:
    template <typename... ARGS>
    trampoline(ARGS&&... args) : T{std::forward<ARGS>(args)...} {}
};

class Buf {
    char* a_;
    int i_;
    const int n_;
    Buf(char* a, int n);

   public:
    bool Write(std::string_view s);
    std::string_view Rewind();

    static Buf of(char* a, int n);
    static Buf of(std::string& s);
};

class File {
    const int fd_;
    Buf buf_;
    File(int fd, Buf buf);
    friend class trampoline<File>;

   public:
    ~File();

    bool Write(std::string_view s);
    bool Flush();

    static std::shared_ptr<File> of(const char* path, Buf buf);
    static std::shared_ptr<File> of(int fd, Buf buf);
};

struct Metadata {
    uint64_t size;
    int64_t ms;
};

class RotatePolicy {
    std::string base_;
    std::string name_;
    std::string ext_;
    std::string slink_;
    int n_;
    int buf_size_;
    RotatePolicy(std::string base, std::string name, std::string ext, int n,
                 int buf_size);

   public:
    std::vector<std::string> Probe() const;
    std::string Path(std::string_view tag) const;
    auto max_files() const {
        return n_;
    }
    auto buf_size() const {
        return buf_size_;
    }
    auto& slink() const {
        return slink_;
    }

    class Builder {
        std::string base_;
        std::string name_;
        std::string ext_{".log"s};
        int n_{6};
        int buf_size_{1024 * 1024};

       public:
        auto& set_base(std::string base) {
            base_ = std::move(base);
            return *this;
        }
        auto& set_name(std::string name) {
            name_ = std::move(name);
            return *this;
        }
        auto& set_ext(std::string ext) {
            ext_ = std::move(ext);
            return *this;
        }
        auto& set_num_files(int n) {
            n_ = n;
            return *this;
        }
        auto& set_buf_size(int buf_size) {
            buf_size_ = buf_size;
            return *this;
        }
        RotatePolicy Build();
    };
};

class SizeRotate {
    const RotatePolicy policy_;
    const uint64_t size_;
    uint64_t written_;
    int i_;
    std::vector<std::string> witness_;
    std::string buf_;
    SizeRotate(RotatePolicy policy, uint64_t size);
    friend class trampoline<SizeRotate>;

   public:
    std::shared_ptr<File> Next();
    bool Spill(Metadata);

    class Builder : public RotatePolicy::Builder {
        uint64_t size_;

       public:
        auto& set_size(uint64_t size) {
            size_ = size;
            return *this;
        }
        std::shared_ptr<SizeRotate> Build();
    };
};

class TimeRotate {
    const RotatePolicy policy_;
    const int64_t span_;
    std::vector<std::string> witness_;
    TimeRotate(RotatePolicy policy, int64_t span);
    friend class trampoline<TimeRotate>;

   public:
    std::shared_ptr<File> Next();
    bool Spill(Metadata);

    class Builder : public RotatePolicy::Builder {
        int64_t span_;

       public:
        auto& set_span(int64_t span) {
            span_ = span;
            return *this;
        }
        std::shared_ptr<TimeRotate> Build();
    };
};

}  // namespace slog