#include "config.h"
#include "test.h"

CLASS_FIELD_ACCESSOR_DECL(slog::NumBytes, n_, int64_t);

int main() {
    using namespace slog;
    auto x = "10"_nb;
    EXPECT_EQ(n_(&x), 10L);

    // invalid
    x = "xyz"_nb;
    EXPECT_EQ(n_(&x), 10L * 1024 * 1024);

    x = "312kb"_nb;
    EXPECT_EQ(n_(&x), 312L * 1024);
    x = "312MB"_nb;
    EXPECT_EQ(n_(&x), 312L * 1024 * 1024);
    x = "312GB"_nb;
    // restrict to 100GB
    EXPECT_EQ(n_(&x), 100L * 1024 * 1024 * 1024);
}
