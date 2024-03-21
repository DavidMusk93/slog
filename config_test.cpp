#include "config.h"
#include "test.h"

int main() {
    using namespace slog;
    EXPECT_EQ("10b"_bs.value(), 10L);
    // invalid
    EXPECT_EQ("xyz"_bs.value(), 10L * 1024 * 1024);
    EXPECT_EQ("312k"_bs.value(), 312L * 1024);
    EXPECT_EQ("312m"_bs.value(), 312L * 1024 * 1024);
    // restrict to 100GB
    EXPECT_EQ("312G"_bs.value(), 100L * 1024 * 1024 * 1024);

    EXPECT_EQ("1h5m1s"_ss.value(), 3600L + 300L + 1);
}
