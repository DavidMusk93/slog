#include "config.h"
#include "test.h"

int main() {
    using namespace slog;
    EXPECT_EQ("10b"_b->value(), 10L);
    // invalid
    EXPECT_EQ("xyz"_b, std::nullopt);
    EXPECT_EQ("312k"_b->value(), 312L * 1024);
    EXPECT_EQ("312m"_b->value(), 312L * 1024 * 1024);
    EXPECT_EQ("312 G"_b->value(), 312L * 1024 * 1024 * 1024);

    EXPECT_EQ("1h5m1s"_s->value(), 3600L + 300 + 1);
    EXPECT_EQ("100'000'000"_s->value(), 100L * 1000 * 1000);
}
