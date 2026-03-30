#ifndef WEBSERVER_HTTP_RESPONSE_H
#define WEBSERVER_HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "buffer/buffer.h"
#include "log/logger.h"

namespace webserver {
    class HttpResponse {
    public:
        explicit HttpResponse(int code = 200) : code_(code) {}

        void set_status_code(int code) { code_ = code; }
        void set_content_type(std::string_view type) { add_header("Content-Type", type); }
        void add_header(std::string_view key, std::string_view value) {
            headers_[std::string(key)] = std::string(value);
        }
        void set_body(std::string body) { body_ = std::move(body); }
        void set_keep_alive(bool keep) { keep_alive_ = keep; }

        void AppendToBuffer(Buffer* output) const;

        std::string GetStatusMsg() const;

        static std::string GetMimeType(const std::string& ext);

    private:

        int code_;
        bool keep_alive_ = false;
        std::unordered_map<std::string, std::string> headers_;
        std::string body_;
    };

} // webserver

#endif //WEBSERVER_HTTP_RESPONSE_H