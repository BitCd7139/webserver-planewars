//
// Created by anonb on 2026/3/28.
//

#include "http_request.h"

#include <charconv>

namespace webserver {
    /* A request example:
     *
     *  POST /bbs/create HTTP/1.1   // request_line
     *  Host: www.abc.com           // header
     *  Content-Length: 34
     *  ...
     *  \r\f
     *  title=welcome&body=welcome,all  // body
     *  empty line
     */

    void HttpRequest::reset() {
        state_ = ParseState::REQUEST_LINE;
        method_.clear();
        path_.clear();
        version_.clear();
        headers_.clear();
        body_.clear();
    }

    size_t HttpRequest::Parse(std::string_view data) {
        size_t used_bytes = 0;
        while (!data.empty() && state_ != ParseState::DONE) {
            size_t pos = data.find("\r\n");
            if (pos == std::string::npos) {
                if (state_ == ParseState::BODY) break;
                return used_bytes;
            }

            std::string_view line = data.substr(0, pos);

            switch (state_) {
                case ParseState::REQUEST_LINE:
                    if (ParseRequestLine(line)) {
                        state_ = ParseState::HEADER;
                    } else {
                        state_ = ParseState::ERROR;
                    }
                    used_bytes += pos + 2;
                    data.remove_prefix(pos + 2);
                    break;

                case ParseState::HEADER:
                    if (line.empty()) {
                        state_ = headers_.count("Content-Length") ?
                        ParseState::BODY : ParseState::DONE;
                    }
                    else {
                        ParseHeaders(line);
                    }
                    used_bytes += pos + 2;
                    data.remove_prefix(pos + 2);
                    break;

                case ParseState::BODY: {
                    auto context_len_str = headers_.find("Content-Length");
                    if (context_len_str == headers_.end()) {
                        LOG_DEBUG("Content-Length header not found, but in BODY state");
                        state_ = ParseState::ERROR;
                        return used_bytes;
                    }
                    size_t content_length = 0;
                    std::from_chars(context_len_str->second.data(), context_len_str->second.data() + context_len_str->second.size(), content_length);

                    if (data.size() >= content_length) {
                        body_ = std::string(data.substr(0, content_length));
                        used_bytes += content_length;
                        state_ = ParseState::DONE;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return used_bytes;
    }

    bool HttpRequest::ParseRequestLine(std::string_view line) {
        size_t pos1 = line.find(' ');
        size_t pos2 = line.rfind(' ');
        if (pos1 == std::string_view::npos || pos2 == std::string_view::npos || pos1 == pos2) {
            return false;
        }
        method_ = std::string(line.substr(0, pos1));
        path_ = std::string(line.substr(pos1 + 1, pos2 - pos1 - 1));
        version_ = std::string(line.substr(pos2 + 1));
        return true;
    }

    std::optional<std::string_view> HttpRequest::get_header(std::string_view name) const {
        if (auto it = headers_.find(std::string(name)); it != headers_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool HttpRequest::ParseHeaders(std::string_view line) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) {
            state_ = ParseState::ERROR;
            return false;
        }

        std::string_view key(line.substr(0, pos));
        std::string_view value(line.substr(pos + 1));

        size_t first_not_space = value.find_first_not_of(" \t");
        if (first_not_space != std::string_view::npos) {
            value.remove_prefix(first_not_space);
        }

        headers_[std::string(key)] = std::string(value);
        return true;
    }

    std::optional<Range> HttpRequest::get_range() const{
        auto it = headers_.find("Range");
        if (it == headers_.end()) return std::nullopt;

        std::string_view val = it->second;
        // "bytes=start-end"
        if (val.find("bytes=") != 0) return std::nullopt;

        val.remove_prefix(6); // "bytes="
        size_t dash_pos = val.find('-');
        if (dash_pos == std::string_view::npos) return std::nullopt;

        Range r;
        std::string_view start_str = val.substr(0, dash_pos);
        std::string_view end_str = val.substr(dash_pos + 1);

        if (!start_str.empty()) {
            std::from_chars(start_str.data(), start_str.data() + start_str.size(), r.start);
        }
        if (!end_str.empty()) {
            std::from_chars(end_str.data(), end_str.data() + end_str.size(), r.end);
        }
        return r;
    }
} // webserver