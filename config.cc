#include "config.h"
#include <array>
#include <optional>
#include <vector>

namespace slog {

using table_t = std::array<int64_t, 256>;

static const auto bytes_unit_table = [] {
    table_t r{};
    r['b'] = 1L;
    r['k'] = 1024L;
    r['m'] = 1024L * 1024;
    r['g'] = 1024L * 1024 * 1024;
    r['B'] = 1L;
    r['K'] = 1024L;
    r['M'] = 1024L * 1024;
    r['G'] = 1024L * 1024 * 1024;
    return r;
}();

static const auto seconds_unit_talbe = [] {
    table_t r{};
    r['s'] = 1L;
    r['m'] = 60L;
    r['h'] = 60L * 60;
    r['d'] = 24L * 60 * 60;
    r['S'] = 1L;
    r['M'] = 60L;
    r['H'] = 60L * 60;
    r['D'] = 24L * 60 * 60;
    return r;
}();

static const auto symbol_table = [] {
    std::array<char, 256> r{};
    // numbers
    for (char c = '0'; c <= '9'; ++c) r[c] = 1;
    // ignore characters
    r[' '] = 2;
    r['\''] = 2;
    return r;
}();

static std::optional<int64_t> FromString(std::string_view s, const table_t& table) {
    int64_t r{}, v{};
    auto p = (const unsigned char*)s.data();
    const int n = s.size();

    for (int i = 0; i < n; ++i) {
        auto x = *p++;
        if (table[x] != 0) {
            // accept unit
            r += v * table[x];
            v = 0;
            continue;
        }
        switch (symbol_table[x]) {
            case 1:
                v *= 10;
                v += x - '0';
                break;
            case 2:
                break;
            case 0:
                // invalid character
                return {};
            default:
                break;
        }
    }

    return r + v;
}

std::optional<Bytes> Bytes::of(std::string_view s) {
    auto r = FromString(s, bytes_unit_table);
    if (!r) return {};
    return Bytes{*r};
}

std::optional<Seconds> Seconds::of(std::string_view s) {
    auto r = FromString(s, seconds_unit_talbe);
    if (!r) return {};
    return Seconds{*r};
}

std::optional<Bytes> operator""_b(const char* p, size_t n) {
    return Bytes::of(std::string_view(p, n));
}

std::optional<Seconds> operator""_s(const char* p, size_t n) {
    return Seconds::of(std::string_view(p, n));
}

}  // namespace slog
