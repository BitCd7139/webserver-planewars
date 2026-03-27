#include "epoller.h"
#include <assert.h>

namespace webserver {
    Epoller::Epoller(int max_event) {
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        assert(epoll_fd_ != -1);
        epoll_events_.resize(max_event);
    }

    bool Epoller::update_channel(channel* channel) {
        return update(EPOLL_CTL_ADD, channel);
    }

    bool Epoller::remove_channel(channel* channel) {
        return update(EPOLL_CTL_DEL, channel);
    }

    bool Epoller::modify_channel(channel *channel) {
        return update(EPOLL_CTL_MOD, channel);
    }

    bool Epoller::update(int operation, channel* channel) const {
        if (channel->get_fd() < 0) {
            return false;
        }

        epoll_event event{};
        event.data.ptr = channel;
        event.events = channel->get_events();
        return 0 == epoll_ctl(epoll_fd_, operation, channel->get_fd(), &event);
    }

    void Epoller::poll(std::vector<channel*>& active_channels, int timeout) {
        int num_events =  epoll_wait(epoll_fd_, epoll_events_.data(), epoll_events_.size(), timeout);
        for (int i = 0; i < num_events; i ++) {
            auto* ch = static_cast<channel*>(epoll_events_[i].data.ptr);
            ch->set_revents(epoll_events_[i].events);
            active_channels.push_back(ch);
        }

        if (num_events == epoll_events_.size()) {
            epoll_events_.resize(epoll_events_.size() * 2);
        }
    }
}