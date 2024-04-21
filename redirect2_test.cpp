#include <sys/poll.h>
#include <unistd.h>
#include <iostream>
#include "log.h"
#include "str.h"

int main() {
    slog::Logger<slog::TimeRotate>::Redirect(STDERR_FILENO, "stderr"s);
    const int n = 0x7fff'ffff;
    for (int i = 0; i < n; ++i) {
        std::cout << '#' << i << std::endl;
        std::cerr << slog::internal::format("{} #{} this is a trivial test\n"sv,
                                            slog::current_seconds(), i);
        poll(nullptr, 0, 500);
    }
}
