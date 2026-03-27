#ifndef WEBSERVER_EPOLLER_H
#define WEBSERVER_EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <error.h>
#include "server/channel.h"

namespace webserver {
    class Epoller {
    public:
        explicit Epoller(int max_event = 1024);

        ~Epoller() {
            close(epoll_fd_);
        }

        Epoller(const Epoller&) = delete;
        Epoller& operator=(const Epoller&) = delete;

        bool update_channel(channel* channel);
        bool remove_channel(channel* channel);
        bool modify_channel(channel* channel);

        void poll(std::vector<channel*>& active_channels, int timeout = -1);

    private:
        bool update(int operation, channel* channel) const;

        int epoll_fd_;
        std::vector<epoll_event> epoll_events_;
    };
}

#endif //WEBSERVER_EPOLLER_H