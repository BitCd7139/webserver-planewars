//
// Created by anonb on 2026/3/28.
//

#include "http_response.h"

namespace webserver {
    /*
     * A response example
     * "HTTP/1.1 200 OK\r\n"
     * "Content-Length: " + std::to_string(body.size()) + "\r\n"
     * "Connection: close\r\n"
     * "\r\n" +
     * body;
     */

    void HttpResponse::AppendToBuffer(Buffer* output) const {
        // HTTP/1.1 200 OK\r\n
        output->Append("HTTP/1.1 ");
        output->Append(std::to_string(code_));
        output->Append(" ");
        output->Append(GetStatusMsg());
        output->Append("\r\n");
        output->Append("Accept-Ranges: bytes\r\n");

        // Key: Value\r\n
        for (const auto&[key, value] : headers_) {
            output->Append(key);
            output->Append(": ");
            output->Append(value);
            output->Append("\r\n");
        }

        // Content-Length Connection
        output->Append("Content-Length: ");
        output->Append(std::to_string(body_.size()));
        output->Append("\r\n");

        if (keep_alive_) {
            output->Append("Connection: keep-alive\r\n");
        } else {
            output->Append("Connection: close\r\n");
        }

        // body
        output->Append("\r\n");
        output->Append(body_);
    }

    std::string HttpResponse::GetStatusMsg() const {
        switch (code_) {
            case 200: return "OK";
            case 206: return "Partial Content";
            case 400: return "Bad Request";
            case 404: return "Not Found";
            case 500: return "Internal Server Error";
            default: return "Unknown";
        }
    }

    std::string HttpResponse::GetMimeType(const std::string& ext) {
        LOG_DEBUG("Getting MIME type for extension: %s", ext.c_str());
        if (ext == ".html" || ext == ".htm") return "text/html";
        if (ext == ".css") return "text/css";
        if (ext == ".js") return "application/javascript";
        if (ext == ".jar") return "application/java-archive";
        if (ext == ".png") return "image/png";
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".ico") return "image/x-icon";
        if (ext == ".gif") return "image/gif";
        if (ext == ".txt") return "text/plain";
        if (ext == ".wasm") return "application/wasm";
        LOG_WARN("Unknown file extension: %s, fallback to text/plain", ext.c_str());
        return "text/plain";
    }

} // webserver