#ifndef WEBSERVER_HTTP_REQUEST_H
#define WEBSERVER_HTTP_REQUEST_H
#include "log/logger.h"
#include <string_view>
#include <functional>
#include <filesystem>
#include <optional>

#include "buffer/buffer.h"

namespace webserver {
    enum class ParseState {
        REQUEST_LINE,
        HEADER,
        BODY,
        DONE,
        ERROR
    };

    struct Range {
        long long start = 0;
        long long end = -1;
    };

    class HttpRequest {
    public:
        [[nodiscard]] size_t Parse(std::string_view data);
        bool is_done() const {return state_ == ParseState::DONE;};
        bool is_error() const {return state_ == ParseState::ERROR;};
        void reset();

        [[nodiscard]] std::string_view methods() const {return method_;};
        [[nodiscard]] std::string_view path() const {return path_;};

        std::optional<std::string_view> get_header(std::string_view key) const;
        bool is_keep_alive() const {
            return get_header("Connection").has_value() &&
                   get_header("Connection")->compare("keep-alive") == 0;
        }

        std::optional<Range> get_range() const;


    private:
        bool ParseRequestLine(std::string_view line);
        bool ParseHeaders(std::string_view line);

        ParseState state_ = ParseState::REQUEST_LINE;
        std::string method_, path_, body_, version_;
        std::unordered_map<std::string, std::string> headers_;

    };
} // webserver

#endif //WEBSERVER_HTTP_REQUEST_H