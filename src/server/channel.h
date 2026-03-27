#ifndef WEBSERVER_CHANNEL_H
#define WEBSERVER_CHANNEL_H
#include <sys/epoll.h>
#include <cstdint>
#include <functional>

namespace webserver {
    class channel {
    public:
        using eventCallback = std::function<void()>;
        channel(int fd): fd_(fd), events_(0) {}

        void set_read_callback(const eventCallback& cb) { readCallback_ = cb; }
        void set_write_callback(const eventCallback& cb) { writeCallback_ = cb; }
        void set_error_callback(const eventCallback& cb) { errorCallback_ = cb; }

        int get_fd() const { return fd_; }
        uint32_t get_events() const { return events_; }

        void set_events(uint32_t events) { events_ = events; }
        void set_revents(uint32_t revents) { real_events_ = revents; }

        void handle_events() {
            if ((real_events_ & EPOLLIN) && readCallback_) {
                readCallback_();
            }
            if ((real_events_ & EPOLLOUT) && writeCallback_) {
                writeCallback_();
            }
            if ((real_events_ & (EPOLLERR | EPOLLHUP)) && errorCallback_) {
                errorCallback_();
            }
        }

        void enable_reading() { events_ |= EPOLLIN; }
        void disable_reading() { events_ &= ~EPOLLIN; }
        void enable_writing() { events_ |= EPOLLOUT; }
        void disable_writing() { events_ &= ~EPOLLOUT; }

    private:
        int fd_;
        uint32_t events_;
        uint32_t real_events_;
        eventCallback readCallback_;
        eventCallback writeCallback_;
        eventCallback errorCallback_;
    };
}


#endif //WEBSERVER_CHANNEL_H