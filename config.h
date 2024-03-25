#pragma once

#include <string>

namespace slog {

class Bytes {
    const int64_t v_;
    Bytes(int64_t v) : v_{v} {}

   public:
    auto value() const {
        return v_;
    }

    static Bytes of(int64_t v);
    static Bytes of(const char* s /* null-terminated */, int64_t _default = 4096);
    static Bytes of(std::string_view s, int64_t _default = 4096);
};

class Seconds {
    const int64_t v_;
    Seconds(int64_t v) : v_{v} {}

   public:
    auto value() const {
        return v_;
    }

    static Seconds of(int64_t v);
    static Seconds of(const char* s /* null-terminated */, int64_t _default = 60);
    static Seconds of(std::string_view s, int64_t _default = 60);
};

Bytes operator""_b(const char* p, size_t n);
Seconds operator""_s(const char* p, size_t n);

}  // namespace slog
