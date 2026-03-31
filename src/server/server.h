#ifndef WEBSERVER_SERVER_H
#define WEBSERVER_SERVER_H

#include <memory>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <mutex>

#include "http/http_conn.h"
#include "server/epoller.h"
#include "log/logger.h"
#include "timer/heap_timer.h"
#include "pool/threadpool.h"

namespace webserver {
    class Server {
    public:
        Server(int port,
            Log::LogLevel log_level = Log::LogLevel::INFO, int threads = 8, int timeout_ms = 60000, std::string doc_root = "./resources")
            :   timer_(timeout_ms),
                port_(port),
                epoller_(std::make_unique<Epoller>()),
                document_root_(doc_root),
                threadpool_(std::make_unique<threadpool>(threads)) {
            Log::instance().init(log_level, true, "./log", ".log");
            GetPwd();
            InitListen();
        };

        void run();

    private:
        void InitListen();

        void GetPwd();

        void HandleNewConnection();

        void HandleHttpRequest(const HttpRequest& request, HttpResponse& response);

        HeapTimer timer_;
        int port_;
        int listen_fd_;
        std::string document_root_;
        std::unique_ptr<channel> listen_channel_;
        std::unique_ptr<Epoller> epoller_;
        std::unordered_map<int, std::unique_ptr<HttpConn>> users_;
        std::unique_ptr<threadpool> threadpool_;
        std::mutex mutex_;
    };
} // webserver

#endif //WEBSERVER_SERVER_H