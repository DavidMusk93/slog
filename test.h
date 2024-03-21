#pragma once

#include <iostream>
#include <string>

using namespace std::literals;

#define EXPECT_TRUE(_x)                                    \
    if (!(_x)) {                                           \
        std::cerr << "unexpected: "sv << #_x << std::endl; \
        return 1;                                          \
    }
#define EXPECT_EQ(x, y) EXPECT_TRUE((x) == (y))
#define EXPECT_NE(x, y) EXPECT_TRUE((x) != (y))

#define CLASS_METHOD_ACCESSOR_DECL(_class, _method, _ret, _args, ...)           \
    template <class M, M m>                                                     \
    struct m_accessor {                                                         \
        friend _ret _method(_class* p, ##__VA_ARGS__) { return (p->*m) _args; } \
    };                                                                          \
                                                                                \
    template struct m_accessor<decltype(&_class::_method), &_class::_method>;   \
    _ret _method(_class* p, __VA_ARGS__);
#define CLASS_FIELD_ACCESSOR_DECL(_class, _filed, _type)                    \
    template <class F, F f>                                                 \
    struct f_accessor {                                                     \
        friend _type _filed(_class* p) { return (p->*f); }                  \
    };                                                                      \
    template struct f_accessor<decltype(&_class::_filed), &_class::_filed>; \
    _type _filed(_class* p);
