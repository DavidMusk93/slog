#include "config.h"

namespace slog {

static int64_t bytes_unit_table[256];
static int64_t seconds_unit_table[256];
static char symbol_table[256];

__attribute__((__constructor__)) static void ctor() {
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

template <typename F>
static int64_t FromString(const unsigned char* p, const int64_t* table,
                          const int64_t _default, F&& end) {
    if (!p) return _default;
    int64_t r{}, v{};

    for (auto x = *p; end(p); x = *++p) {
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

#define FROM_CS(_table) \
    FromString((const unsigned char*)s, _table, _default, [](auto p) { return *p != 0; })
#define FROM_SV(_table)                                                               \
    ({                                                                                \
        auto p = (const unsigned char*)s.data();                                      \
        FromString(p, _table, _default, [e{p + s.size()}](auto p) { return p < e; }); \
    })

Bytes Bytes::of(const char* s, int64_t _default) {
    return FROM_CS(bytes_unit_table);
}

Bytes Bytes::of(std::string_view s, int64_t _default) {
    return FROM_SV(bytes_unit_table);
}

Seconds Seconds::of(int64_t v) {
    return v;
}

Seconds Seconds::of(const char* s, int64_t _default) {
    return FROM_CS(seconds_unit_table);
}

Seconds Seconds::of(std::string_view s, int64_t _default) {
    return FROM_SV(seconds_unit_table);
}

Bytes operator""_b(const char* p, size_t n) {
    return Bytes::of(std::string_view(p, n));
}

Seconds operator""_s(const char* p, size_t n) {
    return Seconds::of(std::string_view(p, n));
}

}  // namespace slog
