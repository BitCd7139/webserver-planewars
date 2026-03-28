#ifndef WEBSERVER_HTTP_CON_H
#define WEBSERVER_HTTP_CON_H
#include "server/channel.h"

namespace webserver {
    class http_conn {
    public:
        http_conn(int fd): fd_(fd), channel_(fd) {
            channel_.set_read_callback(std::bind(&http_conn::handle_read, this));
            channel_.enable_read();
        }

        void add_to_epoller(Epoller& epoller) {
            epoller.update_channel(&channel_);
        }

    private:
        void handle_read();

        int fd_;
        channel channel_;
    };
} // webserver

#endif //WEBSERVER_HTTP_CON_H