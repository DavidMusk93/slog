#include "config.h"
#include <array>
#include <optional>
#include <vector>

namespace slog {

constexpr auto kKB = 1024L;
constexpr auto kMB = 1024 * kKB;
constexpr auto kGB = 1024 * kMB;

constexpr auto kMinute = 60L;
constexpr auto kHour = 60 * kMinute;
constexpr auto kDay = 24 * kHour;

class Parser {
    std::array<int64_t, 256> table_;

   public:
    Parser(std::vector<std::pair<char, int64_t>> v) : table_{} {
        for (auto&& p : v) {
            union {
                char c;
                unsigned char u;
            };
            c = p.first;
            table_[u] = p.second;
        }
    }

    std::optional<int64_t> Parse(std::string_view s) {
        int64_t r{};
        int64_t v{};
        auto p = (unsigned char*)s.data();
        const int n = s.size();

        for (int i = 0; i < n; ++i) {
            if (table_[p[i]] != 0) {
                // accept unit
                r += v * table_[p[i]];
                v = 0;
                continue;
            }

            if (p[i] < '0' || p[i] > '9') {
                // unexpected character
                return {};
            }

            v *= 10;
            v += p[i] - '0';
        }

        return r + v;
    }
};

Bytes Bytes::of(std::string_view s) {
    constexpr auto kDefaultValue = 10 * kMB;
    constexpr auto kUpperValue = 100 * kGB;
    auto p = Parser({
        {'b', 1L },
        {'B', 1L },
        {'k', kKB},
        {'K', kKB},
        {'m', kMB},
        {'M', kMB},
        {'g', kGB},
        {'G', kGB},
    });

    if (auto r = p.Parse(s); r) {
        if (*r > kUpperValue) *r = kUpperValue;
        return {*r};
    }

    return {kDefaultValue};
}

Seconds Seconds::of(std::string_view s) {
    constexpr auto kDefaultValue = 10 * kMinute;
    constexpr auto kUpperValue = 100 * kDay;
    auto p = Parser({
        {'s', 1L     },
        {'S', 1L     },
        {'m', kMinute},
        {'M', kMinute},
        {'h', kHour  },
        {'H', kHour  },
        {'d', kDay   },
        {'D', kDay   },
    });

    if (auto r = p.Parse(s); r) {
        if (*r > kUpperValue) *r = kUpperValue;
        return {*r};
    }

    return {kDefaultValue};
}

Bytes operator""_bs(const char* p, size_t n) {
    return Bytes::of(std::string_view(p, n));
}

Seconds operator""_ss(const char* p, size_t n) {
    return Seconds::of(std::string_view(p, n));
}

}  // namespace slog
