#include "config.h"

namespace slog {

constexpr auto kKB = 1024L;
constexpr auto kMB = 1024 * kKB;
constexpr auto kGB = 1024 * kMB;

NumBytes::NumBytes(int64_t n) : i_{0} {
    n_ = n;
    if (n > 100 * kGB) n_ = 100L * kGB;
}

bool NumBytes::Accept(int64_t bytes) {
    if (i_ + bytes > n_) return false;
    i_ += bytes;
    return true;
}

NumBytes operator""_nb(const char* p, size_t n) {
    constexpr int64_t kDefaultNumBytes = 10 * kMB;
    char* e{};
    auto x = std::strtol(p, &e, 10);
    if (x <= 0) return {kDefaultNumBytes};
    if (p + n == e) return {x};
    switch (*e) {
        case 'k':
        case 'K':
            return {x * kKB};
        case 'm':
        case 'M':
            return {x * kMB};
        case 'g':
        case 'G':
            return {x * kGB};
        default:
            return {kDefaultNumBytes};
    }
}

}  // namespace slog
