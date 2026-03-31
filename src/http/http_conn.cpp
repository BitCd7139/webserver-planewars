//
// Created by anonb on 2026/3/27.
//

#include "http_conn.h"
#include "log/logger.h"
#include <string_view>
#include <optional>
#include <charconv>

namespace webserver {
    void HttpConn::HandleRead() {
        timer_->adjust(fd_, timer_->timeout_ms_);
        int err;
        ssize_t bytes_read = read_buffer_.ReadFd(fd_, &err);
        if (bytes_read == 0) {
            HandleClose();
        }
        else if (bytes_read < 0) {
            //TODO: 处理 error
        }
        else {
            std::string_view data(read_buffer_.peek(), read_buffer_.readable_bytes());
            size_t consumed = request_.Parse(data);
            read_buffer_.Retrieve(consumed);
            timer_->adjust(fd_, timer_->timeout_ms_);


            if (request_.is_error()) {
                // 400 Bad Request
                response_.set_status_code(400);
                response_.set_body("400 Bad Request");
                response_.AppendToBuffer(&write_buffer_);
                HandleWrite();
                return;
            }

            // 收到完整包
            if (request_.is_done()) {
                response_.set_status_code(200);
                response_.set_keep_alive(request_.is_keep_alive());

                if (http_handler_) {
                    http_handler_(request_, response_);
                }
                else {
                    response_.set_status_code(404);
                    response_.set_body("404 Not Found");
                }

                response_.AppendToBuffer(&write_buffer_);
                request_.reset();
                HandleWrite();
                return;
            }
        }
    }

    void HttpConn::HandleWrite() {
        std::string_view data(write_buffer_.peek(), write_buffer_.readable_bytes());
        if (data.empty()) {return;}

        ssize_t n = write(fd_, data.data(), data.size());

        if (n > 0) {
            write_buffer_.Retrieve(n);


            if (write_buffer_.readable_bytes() == 0) {
                channel_.disable_writing();
                epoller_->modify_channel(&channel_);

                if (is_keep_alive_) {
                    request_.reset();
                    read_buffer_.RetrieveAll();
                    write_buffer_.RetrieveAll();
                    channel_.enable_reading();
                    channel_.enable_et();
                    channel_.enable_oneshot();
                    epoller_->modify_channel(&channel_);
                }
                else{
                    HandleClose();
                }
            }
            else {
                channel_.enable_writing();
                epoller_->modify_channel(&channel_);
            }
        }
        else {
            if (errno != EAGAIN) {
                HandleClose();
            }
        }
    }

    void HttpConn::HandleClose() {
        LOG_INFO("Client disconnected from fd: %d", fd_);
        if (close_callback_) {
            close_callback_();
        }
    }
} // webserver