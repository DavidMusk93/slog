#include "str.h"
#include "test.h"

int main() {
    using namespace slog::internal;
    EXPECT_EQ(format("{{"sv), "{"sv);
    EXPECT_EQ(format("this {} a test"sv, "is"sv), "this is a test"sv);
}
