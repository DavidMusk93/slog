#include "log.h"
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <unistd.h>
#include <atomic>
#include <mutex>
#include "file.h"
#include "iter.h"

namespace slog {

int64_t current_seconds();

class event_handler : public slog::internal::dl_node {
   public:
    virtual ~event_handler() = default;
    virtual bool on_event() = 0;
    virtual int fd() = 0;
};

template <typename LOGGER>
class proxy : public event_handler {
    int source_;
    std::shared_ptr<LOGGER> sink_;

   public:
    proxy(int fd, std::shared_ptr<LOGGER> logger)
        : source_{fd}, sink_{std::move(logger)} {}
    ~proxy() {
        close(source_);
    }

    bool on_event() override {
        char buf[1024];
        int n = read(source_, buf, sizeof(buf));
        if (n <= 0) {
            if (errno == EAGAIN /* unlikely */ || errno == EINTR) return true;
            return false;
        }
        sink_->Log({buf, (size_t)n}, {current_seconds()});
        return true;
    }

    int fd() override {
        return source_;
    }
};

template <typename LOGGER>
proxy(int, std::shared_ptr<LOGGER>) -> proxy<LOGGER>;

class async_logger {
    int poller_;
    int term_;
    internal::dl_node handlers_;
    std::mutex mutex_;
    pthread_t worker_;
    std::atomic_int64_t seconds_;

    void tick() {
        struct timeval tv {};
        gettimeofday(&tv, nullptr);
        seconds_.store(tv.tv_sec, std::memory_order_relaxed);
    }

    void run() {
        const int max_events = 8;
        struct epoll_event events[max_events];
        for (;;) {
            int n = epoll_wait(poller_, events, max_events, 1000);
            if (n == 0) {
                tick();
                continue;
            }
            if (n == -1) {
                if (errno == EINTR) continue;
                // unrecoverable
                break;
            }
            for (int i = 0; i < n; ++i) {
                if (events[i].data.ptr == &term_) return;
                auto h = (event_handler*)events[i].data.ptr;
                if (!h->on_event()) {
                    epoll_ctl(poller_, EPOLL_CTL_DEL, h->fd(), nullptr);
                    std::scoped_lock lock{mutex_};
                    delete h;
                }
            }
            tick();
        }
    }

   public:
    async_logger() {
        poller_ = epoll_create1(EPOLL_CLOEXEC);
        term_ = eventfd(0, EFD_CLOEXEC);

        struct epoll_event ev {};
        ev.events = EPOLLIN;
        ev.data.ptr = &term_;
        epoll_ctl(poller_, EPOLL_CTL_ADD, term_, &ev);

        tick();

        pthread_create(
            &worker_, nullptr,
            +[](void* p) {
                static_cast<async_logger*>(p)->run();
                return p;
            },
            this);
    }

    ~async_logger() {
        eventfd_write(term_, 1);
        pthread_join(worker_, nullptr);

        handlers_.iter([](auto p) {
            delete static_cast<event_handler*>(p);
            return 0;
        });

        close(term_);
        close(poller_);
    }

    template <typename LOGGER>
    void redirect(int fd, std::shared_ptr<LOGGER> logger) {
        int fds[2]{};
        pipe2(fds, O_NONBLOCK | O_CLOEXEC);
        dup2(fds[1], fd);
        close(fds[1]);

        struct epoll_event ev {};
        ev.events = EPOLLIN;
        auto p = new proxy{fds[0], std::move(logger)};
        ev.data.ptr = p;
        std::scoped_lock lock{mutex_};
        handlers_.on(p);
        epoll_ctl(poller_, EPOLL_CTL_ADD, fds[0], &ev);
    }

    auto current_seconds() const {
        return seconds_.load(std::memory_order_relaxed);
    }

    static auto& instance() {
        static async_logger obj;
        return obj;
    }
};

int64_t current_seconds() {
    return async_logger::instance().current_seconds();
}

template <typename ROTATE_POLICY>
void Logger<ROTATE_POLICY>::Log(std::string_view s, metadata m) {
    f_->Write(s);
    if (p_->Spill({s.size(), m.event_time})) {
        f_ = p_->Next();
    }
}

template <>
void Logger<SizeRotate>::Redirect(int fd, std::string name) {
    SizeRotate::Builder builder;
    builder.set_size("100m"_b)
        .set_name(std::move(name))
        .set_base("."s)
        .set_buf_size("1k"_b)
        .set_num_files(6);
    async_logger::instance().redirect(fd, of(builder.Build()));
}

template <>
void Logger<SizeRotate>::Redirect(int fd, std::shared_ptr<SizeRotate> p) {
    async_logger::instance().redirect(fd, of(std::move(p)));
}

template <>
void Logger<TimeRotate>::Redirect(int fd, std::string name) {
    TimeRotate::Builder builder;
    builder.set_span("1h"_s)
        .set_name(std::move(name))
        .set_base("."s)
        .set_buf_size("1k"_b)
        .set_num_files(6);
    async_logger::instance().redirect(fd, of(builder.Build()));
}

template <>
void Logger<TimeRotate>::Redirect(int fd, std::shared_ptr<TimeRotate> p) {
    async_logger::instance().redirect(fd, of(std::move(p)));
}

template class Logger<SizeRotate>;
template class Logger<TimeRotate>;

}  // namespace slog
