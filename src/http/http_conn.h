#ifndef WEBSERVER_HTTP_CON_H
#define WEBSERVER_HTTP_CON_H
#include "server/channel.h"
#include "server/epoller.h"
#include "buffer/buffer.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace webserver {
    using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

    class HttpConn {
    public:
        HttpConn(int fd, Epoller* epoller)
            : fd_(fd), channel_(fd), epoller_(epoller) {

            channel_.set_read_callback([this]() { HandleRead(); });
            channel_.set_write_callback([this]() { HandleWrite(); });
            channel_.set_error_callback([this]() { HandleClose(); });

            channel_.enable_reading();
            epoller_->add_channel(&channel_);
        }

        ~HttpConn() {
            epoller_->remove_channel(&channel_);
            close(fd_);
        }

        using CloseCallback = std::function<void()>;
        void SetCloseCallback(CloseCallback close_callback) {
            close_callback_ = std::move(close_callback);
        }
        void SetHttpHandler(HttpHandler http_handler) {
            http_handler_ = std::move(http_handler);
        }

    private:
        void HandleRead();
        void HandleWrite();
        void HandleClose();
        CloseCallback close_callback_;

        int fd_;
        channel channel_;
        Epoller* epoller_;

        Buffer read_buffer_;
        Buffer write_buffer_;

        bool is_keep_alive_ = false;
        HttpRequest request_;
        HttpResponse response_;
        HttpHandler http_handler_;
    };
} // webserver

#endif //WEBSERVER_HTTP_CON_H