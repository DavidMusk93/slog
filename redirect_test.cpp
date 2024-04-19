#include <unistd.h>
#include <iostream>
#include "log.h"
#include "str.h"

int main() {
    slog::Logger<slog::SizeRotate>::Redirect(STDOUT_FILENO, "stdout"s);
    std::cout << "hello world!"sv << std::endl;
    std::cout << "this is #2 line!"sv << std::endl;
    std::cout << "this is #3 line!"sv << std::endl;
    std::cout << "this is #4 line!"sv << std::endl;
    std::cout << "this is final line!"sv << std::endl;
}
