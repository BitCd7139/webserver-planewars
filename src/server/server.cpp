#include "server.h"

#include <cstring>

#include "http/http_conn.h"
#include "util/get_root.h"
#include <charconv>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>


namespace webserver {
    void Server::InitListen() {
        listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

        int opt = 1;
        setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);

        bind(listen_fd_, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
        listen(listen_fd_, 128);
        LOG_INFO("Server started on port %d", port_);

        listen_channel_ = std::make_unique<channel>(listen_fd_);
        listen_channel_->set_read_callback([this]() {
            HandleNewConnection();
        });
        listen_channel_->enable_reading();
        listen_channel_->enable_et();

        epoller_->add_channel(listen_channel_.get());
    }

    void Server::HandleNewConnection() {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        while (true) {
            int client_fd = accept4(listen_fd_, reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

            if (client_fd > 0) {
                auto conn = std::make_unique<HttpConn>(client_fd, epoller_.get(), &timer_);
                timer_.add(client_fd, timer_.timeout_ms_, [this, client_fd]() {
                    LOG_INFO("Connection timeout, fd: %d", client_fd);
                    std::lock_guard<std::mutex> lock(mutex_);
                    users_.erase(client_fd);
                });
                conn->SetCloseCallback([this, client_fd]() {
                    LOG_INFO("Connection closed, fd: %d", client_fd);
                    std::lock_guard<std::mutex> lock(mutex_);
                    users_.erase(client_fd);
                });

                conn->SetHttpHandler([this](const HttpRequest& req, HttpResponse& res) {
                    this->HandleHttpRequest(req, res);
                });

                std::lock_guard<std::mutex> lock(mutex_);
                users_[client_fd] = std::move(conn);
                LOG_INFO("New connection from client on port %d", client_fd);
            }
            else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                else {
                    LOG_ERROR("Accept error: %s", std::strerror(errno));
                    break;
                }
            }
        }
    }

    void Server::run() {
        while (true) {
            int timeout_ms = timer_.GetNextTick();
            std::vector<channel*> active_channels;
            epoller_->Poll(active_channels, timeout_ms);
            for (auto* channel : active_channels) {
                if (channel == listen_channel_.get()) {
                    channel->handle_events();
                }
                else {
                    threadpool_->enqueue([channel]() {
                        channel->handle_events();
                    });
                }
            }
        }
    }

void Server::HandleHttpRequest(const HttpRequest& req, HttpResponse& res) {
        namespace fs = std::filesystem;

        // index.html
        std::string req_path = std::string(req.path());
        if (req_path == "/") {
            req_path = "/index.html";
        }

        try {
            // 1. 路径解析与安全校验 (防目录穿越)
            fs::path base_path = fs::canonical(document_root_);
            fs::path target_path = fs::weakly_canonical(base_path / req_path.substr(1));

            auto[base_it, target_it] = std::mismatch(base_path.begin(), base_path.end(), target_path.begin());
            if (base_it != base_path.end()) {
                res.set_status_code(403);
                res.set_body("<h1>403 Forbidden</h1>");
                return;
            }

            // 2. 检查文件是否存在
            if (!fs::exists(target_path) || !fs::is_regular_file(target_path)) {
                res.set_status_code(404);
                res.set_body("<h1>404 Not Found</h1>");
                return;
            }

            // 3. 获取文件总大小，准备解析 Range
            size_t file_size = fs::file_size(target_path);
            size_t start = 0;
            size_t end = file_size - 1;
            bool is_range_request = false;

            // 提取 Range 请求头 (格式如: "bytes=0-1048575")
            auto range_header = req.get_header("Range"); // 依赖我们在 HttpRequest 中修改的忽略大小写的 get_header
            if (range_header.has_value()) {
                std::string_view range_str = range_header.value();

                if (range_str.substr(0, 6) == "bytes=") {
                    range_str.remove_prefix(6);
                    size_t dash_pos = range_str.find('-');

                    if (dash_pos != std::string_view::npos) {
                        // 解析起始点 (start)
                        if (dash_pos > 0) {
                            std::from_chars(range_str.data(), range_str.data() + dash_pos, start);
                        }

                        // 解析终止点 (end)，如果客户端传了的话
                        if (dash_pos + 1 < range_str.size()) {
                            std::from_chars(range_str.data() + dash_pos + 1, range_str.data() + range_str.size(), end);
                        }

                        // 边界检查：如果请求的 start 已经超过文件大小
                        if (start >= file_size) {
                            res.set_status_code(416); // Range Not Satisfiable
                            res.add_header("Content-Range", "bytes */" + std::to_string(file_size));
                            res.set_body("<h1>416 Range Not Satisfiable</h1>");
                            return;
                        }

                        // 防止 end 越界
                        if (end >= file_size) {
                            end = file_size - 1;
                        }

                        is_range_request = true;
                    }
                }
            }

            // 4. 按需读取文件内容
            size_t read_len = end - start + 1;
            std::ifstream file(target_path, std::ios::binary);
            if (!file.is_open()) {
                res.set_status_code(500);
                res.set_body("<h1>500 Internal Server Error</h1>");
                return;
            }

            // 指针跳转到 start 位置，并仅读取 read_len 长度的数据
            file.seekg(start);
            std::string buffer(read_len, '\0');
            file.read(buffer.data(), read_len);

            // 5. 组装响应状态码和 Header
            if (is_range_request) {
                res.set_status_code(206); // 核心：206 Partial Content
                std::string content_range = "bytes " + std::to_string(start) + "-" +
                                            std::to_string(end) + "/" + std::to_string(file_size);
                res.add_header("Content-Range", content_range);
            } else {
                res.set_status_code(200);
            }

            // 必须声明服务器支持 bytes 级别的 Range，让浏览器放心使用分块
            res.add_header("Accept-Ranges", "bytes");

            // 6. 设置其他常规属性
            res.set_content_type(HttpResponse::GetMimeType(target_path.extension().string()));
            res.set_keep_alive(req.is_keep_alive());

            // 使用 std::move 避免对 buffer 进行无意义的深度拷贝，进一步提升性能
            res.set_body(std::move(buffer));

        } catch (const std::exception& e) {
            LOG_ERROR("File system error: %s", e.what());
            res.set_status_code(500);
            res.set_body("<h1>500 Internal Server Error</h1>");
        }
    }

    void Server::GetPwd() {
        std::string root = GetProjectRoot();
        LOG_INFO("Current project root: %s", root.c_str());
        document_root_ = root + "/resources";
    }

} // webserver