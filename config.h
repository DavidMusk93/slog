#pragma once

#include <optional>
#include <string>

namespace slog {

class Bytes {
    const int64_t v_;
    Bytes(int64_t v) : v_{v} {}

   public:
    auto value() const {
        return v_;
    }

    static std::optional<Bytes> of(std::string_view s);
};

class Seconds {
    const int64_t v_;
    Seconds(int64_t v) : v_{v} {}

   public:
    auto value() const {
        return v_;
    }

    static std::optional<Seconds> of(std::string_view s);
};

std::optional<Bytes> operator""_b(const char* p, size_t n);
std::optional<Seconds> operator""_s(const char* p, size_t n);

}  // namespace slog
