#ifndef WEBSERVER_SERVER_H
#define WEBSERVER_SERVER_H

#include <memory>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include "http/http_conn.h"
#include "server/epoller.h"
#include "log/logger.h"

namespace webserver {
    class Server {
    public:
        Server(int port, Log::LogLevel log_level = Log::LogLevel::INFO, std::string doc_root = "./resources") :
        port_(port), epoller_(std::make_unique<Epoller>()), document_root_(doc_root) {
            Log::instance().init(log_level, true, "./log", ".log");
            GetPwd();
            InitListen();
        };

        void run() const;

    private:
        void InitListen();

        void GetPwd();

        void HandleNewConnection();

        void HandleHttpRequest(const HttpRequest& request, HttpResponse& response);

        int port_;
        int listen_fd_;
        std::string document_root_;
        std::unique_ptr<channel> listen_channel_;
        std::unique_ptr<Epoller> epoller_;
        std::unordered_map<int, std::unique_ptr<HttpConn>> users_;
    };
} // webserver

#endif //WEBSERVER_SERVER_H