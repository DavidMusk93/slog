#include "str.h"
#include <string.h>

namespace slog::internal {

bool starts_with(std::string_view s, std::string_view prefix) {
    if (prefix.empty()) return true;

    if (s.size() < prefix.size()) return false;
    return memcmp(s.data(), prefix.data(), prefix.size()) == 0;
}

bool ends_with(std::string_view s, std::string_view suffix) {
    if (suffix.empty()) return true;

    if (s.size() < suffix.size()) return false;
    return memcmp(s.data() + s.size() - suffix.size(), suffix.data(), suffix.size()) == 0;
}

bool contains(std::string_view s, std::string_view sub) {
    if (s.size() < sub.size()) return false;
    return memmem(s.data(), s.size(), sub.data(), sub.size()) != nullptr;
}

template <typename ON_ESCAPE, typename ON_SLOT>
static void parse(std::string_view s, ON_ESCAPE&& on_escape, ON_SLOT&& on_slot) {
    const int n = s.size();
    int lstate, rstate;
    lstate = rstate = -1;

    auto forward = [&](int& state, int cur, auto&& op) -> bool {
        if (state == -1) {
            state = cur;
            op();
            return true;
        }
        if (state + 1 != cur) {
            // invalid format
            return false;
        }
        on_escape(state, cur);
        state = -1;
        return true;
    };

    for (int i = 0; i < n; ++i) {
        auto c = s[i];
        if (c == '{') {
            if (!forward(lstate, i, [] {})) break;
        } else if (c == '}') {
            if (!forward(rstate, i, [&] {
                    if (lstate != -1) {
                        on_slot(lstate, rstate);
                        lstate = rstate = -1;
                    }
                }))
                break;
        }
    }
}

fmt_parser::fmt_parser(std::string_view s) : s_{s} {
    spans_.reserve(8);
    parse(
        s_,
        [&](int l, int r) {
            spans_.push_back({l, r});
        },
        [&](int l, int r) {
            // FIXME: test position
            slots_.set(spans_.size());
            spans_.push_back({l, r});
        });
}

std::string fmt_parser::apply(std::vector<std::string_view> args) const {
    const int n = spans_.size();
    int p{};
    std::ostringstream ss;

    args.resize(slots_.count());
    auto it = args.begin();

    for (int i = 0; i < n; ++i) {
        ss << s_.substr(p, spans_[i].l - p);
        if (slots_.test(i)) {
            ss << *it++;
        } else {
            ss << s_[spans_[i].l];
        }
        p = spans_[i].r + 1;
    }

    ss << s_.substr(p);
    return ss.str();
}

fmt_parser* parser_cache::get(std::string_view fmt) {
    std::shared_ptr<fmt_parser> p;
    {
        std::shared_lock<std::shared_mutex> lock{m};
        auto it = parser_map.find(fmt);
        if (it != parser_map.end()) {
            p = it->second;
        }
    }
    if (!p) {
        p = std::make_shared<fmt_parser>(fmt);
        auto k = h.hold(std::string{fmt});
        {
            std::unique_lock<std::shared_mutex> lock{m};
            parser_map.emplace(*k, p);
        }
    }
    return p.get();
}

thread_local str_holder args_holder;
parser_cache cache;

}  // namespace slog::internal
