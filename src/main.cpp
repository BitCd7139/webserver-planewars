#include<iostream>
#include"pool/threadpool.h"
#include"log/logger.h"
#include"server/epoller.h"
using namespace std;

void test_func(int a, int b) {
    printf("Task running, id: %d\n", a);
    LOG_DEBUG("Task running, id: %d", a);
}

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>
#include "server/epoller.h"

using namespace webserver;

int main() {
    // 1. 创建监听 Socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 128);
    std::cout << "Server started on port 8888..." << std::endl;

    Epoller epoller;

    channel accept_channel(listen_fd);
    accept_channel.set_read_callback([&]() {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);
        std::cout << "New connection accepted: fd " << client_fd << std::endl;

        auto* conn_ch = new channel(client_fd);
        conn_ch->set_read_callback([conn_ch]() {
            char buf[1024];
            ssize_t n = read(conn_ch->get_fd(), buf, sizeof(buf));
            if (n > 0) {
                std::cout << "Received: " << std::string(buf, n) << std::endl;
                write(conn_ch->get_fd(), buf, n); // Echo 回去
            } else {
                close(conn_ch->get_fd());
                std::cout << "Client closed." << std::endl;
            }
        });
        conn_ch->enable_reading();
        epoller.update_channel(conn_ch);
    });

    accept_channel.enable_reading();
    epoller.update_channel(&accept_channel);

    // 3. 事件循环
    while (true) {
        std::vector<channel*> active_channels;
        epoller.poll(active_channels, -1); // 阻塞等待事件

        for (auto ch : active_channels) {
            ch->handle_events(); // 处理读写回调
        }
    }

    return 0;
}