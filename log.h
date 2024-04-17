#pragma once

#include <memory>
#include <string_view>
#include "file.h"

namespace slog {

template <typename ROTATE_POLICY>
class Logger {
    struct metadata {
        int64_t event_time;  // ms
    };
    std::shared_ptr<ROTATE_POLICY> p_;
    std::shared_ptr<File> f_;

    Logger(std::shared_ptr<ROTATE_POLICY> p) : p_{std::move(p)}, f_{p_->Next()} {}
    friend class trampoline<Logger>;

   public:
    void Log(std::string_view s, metadata m);

    static std::shared_ptr<Logger> of(std::shared_ptr<ROTATE_POLICY> p) {
        return std::make_shared<trampoline<Logger>>(std::move(p));
    }
};

extern int pid;
extern thread_local int tid;

}  // namespace slog
