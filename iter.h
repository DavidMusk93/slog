#pragma once

#include <sys/dir.h>
#include <string>

namespace slog::internal {

template <typename F, typename ON_ERROR>
void iter(const std::string& path, F&& f, ON_ERROR&& on_error) {
    {
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            on_error(path);
            return;
        }
        struct dirent* e{};
        while ((e = readdir(dir))) {
            if (f(e) != 0) break;
        }
        closedir(dir);
    }
}

}  // namespace slog::internal
