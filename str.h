#pragma once

#include <bitset>
#include <deque>
#include <map>
#include <memory>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <vector>

using namespace std::literals;

namespace slog::internal {

bool starts_with(std::string_view s, std::string_view prefix);
bool ends_with(std::string_view s, std::string_view suffix);
bool contains(std::string_view s, std::string_view sub);

class fmt_parser {
    struct span {
        int l, r;
    };
    std::string_view s_;
    std::bitset<64> slots_;
    std::vector<span> spans_;

   public:
    fmt_parser(std::string_view s);
    std::string apply(std::vector<std::string_view> args) const;
};

template <typename T>
class holder {
    std::deque<T> storage_;

   public:
    T* hold(T t) {
        storage_.push_back(std::move(t));
        return &storage_.back();
    }
    void reset() {
        storage_.clear();
    }
};

using str_holder = holder<std::string>;

template <typename T>
class stringify {
    str_holder& h_;
    T t_;

   public:
    stringify(str_holder& h, T t) : h_{h}, t_{std::move(t)} {}
    std::string_view operator()() const {
        std::ostringstream ss;
        ss << t_;
        return *h_.hold(ss.str());
    }
};
template <typename T>
class stringify<const T&> {
    str_holder& h_;
    const T& t_;

   public:
    stringify(str_holder& h, const T& t) : h_{h}, t_{t} {}
    std::string_view operator()() const {
        std::ostringstream ss;
        ss << t_;
        return *h_.hold(ss.str());
    }
};
template <>
class stringify<char> {
    char c_;

   public:
    stringify(str_holder&, char c) : c_{c} {}
    std::string_view operator()() const {
        return {&c_, 1};
    }
};
template <>
class stringify<std::string_view> {
    std::string_view s_;

   public:
    stringify(str_holder&, std::string_view s) : s_{s} {}
    std::string_view operator()() const {
        return s_;
    }
};

template <typename T,
          typename = std::enable_if_t<std::is_constructible_v<std::string_view, T>>>
stringify(str_holder&, T&&) -> stringify<std::string_view>;

extern thread_local str_holder args_holder;
struct parser_cache {
    std::shared_mutex m;
    str_holder h;
    std::map<std::string_view, std::shared_ptr<fmt_parser>> parser_map;
    fmt_parser* get(std::string_view fmt);
};
extern parser_cache cache;

template <typename... ARGS>
std::string format(std::string_view fmt, ARGS&&... args) {
    std::vector<std::string_view> v;
    args_holder.reset();

    v.reserve(sizeof...(args));
    (v.push_back(stringify{args_holder, std::forward<ARGS>(args)}()), ...);

    auto p = cache.get(fmt);
    return p->apply(std::move(v));
}

}  // namespace slog::internal
