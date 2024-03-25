#include "config.h"

namespace slog {

static int64_t bytes_unit_table[256];
static int64_t seconds_unit_table[256];
static char symbol_table[256];

__attribute__((__constructor__)) void init() {
    {
        auto a = bytes_unit_table;
        a[(int)'b'] = 1L;
        a[(int)'k'] = 1024L;
        a[(int)'m'] = 1024L * 1024;
        a[(int)'g'] = 1024L * 1024 * 1024;
        a[(int)'B'] = 1L;
        a[(int)'K'] = 1024L;
        a[(int)'M'] = 1024L * 1024;
        a[(int)'G'] = 1024L * 1024 * 1024;
    }
    {
        auto a = seconds_unit_table;
        a[(int)'s'] = 1L;
        a[(int)'m'] = 60L;
        a[(int)'h'] = 60L * 60;
        a[(int)'d'] = 24L * 60 * 60;
        a[(int)'S'] = 1L;
        a[(int)'M'] = 60L;
        a[(int)'H'] = 60L * 60;
        a[(int)'D'] = 24L * 60 * 60;
    }
    {
        auto a = symbol_table;
        // numbers
        for (int i = '0'; i <= '9'; ++i) a[i] = 1;
        // ignore characters
        a[(int)' '] = 2;
        a[(int)'\''] = 2;
    }
}

static int64_t FromString(const char* s, const int64_t* table, const int64_t _default) {
    int64_t r{}, v{};
    auto p = (const unsigned char*)s;

    for (auto x = *p; x != 0; x = *++p) {
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
                return _default;
            default:
                break;
        }
    }

    return r + v;
}

static int64_t FromString(std::string_view s, const int64_t* table,
                          const int64_t _default) {
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
                return _default;
            default:
                break;
        }
    }

    return r + v;
}

Bytes Bytes::of(int64_t v) {
    return v;
}

Bytes Bytes::of(const char* s, int64_t _default) {
    return FromString(s, bytes_unit_table, _default);
}

Bytes Bytes::of(std::string_view s, int64_t _default) {
    return FromString(s, bytes_unit_table, _default);
}

Seconds Seconds::of(int64_t v) {
    return v;
}

Seconds Seconds::of(const char* s, int64_t _default) {
    return FromString(s, seconds_unit_table, _default);
}

Seconds Seconds::of(std::string_view s, int64_t _default) {
    return FromString(s, seconds_unit_table, _default);
}

Bytes operator""_b(const char* p, size_t n) {
    return Bytes::of(std::string_view(p, n));
}

Seconds operator""_s(const char* p, size_t n) {
    return Seconds::of(std::string_view(p, n));
}

}  // namespace slog
