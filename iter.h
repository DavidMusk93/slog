#pragma once

#include <sys/dir.h>
#include <string>

namespace slog::internal {

class dl_node {
    dl_node* prev_;
    dl_node* next_;

   public:
    dl_node() {
        prev_ = next_ = this;
    }
    ~dl_node() {
        if (prev_ != this) off();
    }
    auto& on(dl_node* n) {
        /*
           this -> next
                <-
         */
        next_->prev_ = n;
        n->prev_ = this;
        /*
           this ->      next
                <- n <-
         */
        n->next_ = next_;
        next_ = n;
        /*
           this -> n -> next
                <-   <-
         */
        return *n;
    }
    void off() {
        /*
           prev -> this -> next
                <-      <-
         */
        prev_->next_ = next_;
        next_->prev_ = prev_;
        /*
           prev -> next
                <-
         */
    }

    template <typename F>
    void iter(F&& f) {
        auto n = next_;
        while (n != this) {
            if (f(n) != 0) break;
            n = n->next_;
        }
    }
};

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
