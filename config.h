#pragma once

#include <string>

namespace slog {
class NumBytes {
    int64_t i_;
    int64_t n_;
    NumBytes(int64_t n);

   public:
    bool Accept(int64_t bytes);
    friend NumBytes operator""_nb(const char* p, size_t n);
};
NumBytes operator""_nb(const char* p, size_t n);

}  // namespace slog
