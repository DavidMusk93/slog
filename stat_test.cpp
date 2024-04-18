#include <sys/stat.h>
#include <iostream>
#include "iter.h"
#include "str.h"

using namespace std::literals;

static std::ostream& operator<<(std::ostream& os, struct timespec ts) {
    auto nano = std::to_string(ts.tv_nsec);
    if (nano.size() < 9) nano = std::string(9 - nano.size(), '0') + nano;
    os << ts.tv_sec << '.' << nano;
    return os;
}

static std::ostream& operator<<(std::ostream& os, struct stat& st) {
    os << "stat{atim:"sv << st.st_atim << ",ctim:"sv << st.st_ctim << ",mtim:"sv
       << st.st_mtim << ",size:"sv << st.st_size << '}';
    return os;
}

int main(int argc, char* argv[]) {
    std::string path = argc < 2 ? "/tmp"s : argv[1];
    slog::internal::iter(
        path,
        [&](const dirent* e) {
            if (e->d_type != DT_REG) return 0;
            if (!slog::internal::ends_with(e->d_name, ".log"sv)) return 0;
            auto f = path + '/' + e->d_name;
            struct stat st {};
            stat(f.c_str(), &st);
            std::cout << f << " ->" << st << std::endl;
            return 0;
        },
        [](auto) {});
}
