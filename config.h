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

    static Bytes of(std::string_view s);
};

class Seconds {
    const int64_t v_;
    Seconds(int64_t v) : v_{v} {}

   public:
    auto value() const {
        return v_;
    }

    static Seconds of(std::string_view s);
};

Bytes operator""_b(const char* p, size_t n);
Seconds operator""_s(const char* p, size_t n);

}  // namespace slog
