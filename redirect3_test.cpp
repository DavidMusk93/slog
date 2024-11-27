#include <fcntl.h>
#include <sys/poll.h>
#include <unistd.h>
// #include <iostream>
#include "log.h"

int main(int argc, char* argv[]) {
    std::string_view path, name;
    for (int i = 1; i < argc; ++i) {
        std::string_view t = argv[i];
        if (t == "-p" || t == "--path") {
            if (++i == argc) break;
            path = argv[i];
        } else if (t == "-n" || t == "--name") {
            if (++i == argc) break;
            name = argv[i];
        }
    }

    if (path.empty()) path = ".";
    if (name.empty()) name = "stdout";

    using namespace slog;
    SizeRotate::Builder builder;
    builder.set_size("10m"_b)
        .set_base(std::string{path})
        .set_name(std::string{name})
        .set_buf_size("1k"_b)
        .set_num_files(3);

    Logger<SizeRotate>::Redirect(STDOUT_FILENO, builder.Build());

    char buf[1024];

    int flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    struct pollfd pfd {
        .fd = STDIN_FILENO, .events = POLLIN, .revents = 0
    };

    // std::cout.rdbuf()->pubsetbuf(nullptr, 0);
    for (;;) {
        int rc = poll(&pfd, 1, -1);
        if (rc < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (pfd.revents & POLLIN) {
            auto n = read(pfd.fd, buf, 1024);
            if (n > 0) {
                // std::cout.write(buf, n);
                write(STDOUT_FILENO, buf, n);
            }
        }
    }
}
