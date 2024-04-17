#include "log.h"
#include "file.h"

namespace slog {

template <typename ROTATE_POLICY>
void Logger<ROTATE_POLICY>::Log(std::string_view s, metadata m) {
    f_->Write(s);
    if (p_->Spill({s.size(), m.event_time})) {
        f_ = p_->Next();
    }
}

template class Logger<SizeRotate>;

}  // namespace slog
