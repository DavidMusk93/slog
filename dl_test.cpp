#include <array>
#include <iostream>
#include "iter.h"
#include "test.h"

struct foo : slog::internal::dl_node {
    int i;
    foo(int i) : i{i} {}
};

int main() {
    slog::internal::dl_node stub;
    foo a{1}, b{2}, c{3};
    stub.on(&a).on(&b).on(&c);

    int i{};
    std::array list{&a, &b, &c};
    stub.iter([&](auto p) {
        EXPECT_EQ(list[i++], p);
        std::cout << static_cast<foo*>(p)->i << std::endl;
        return 0;
    });
}
