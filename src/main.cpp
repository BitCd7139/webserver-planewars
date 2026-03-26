#include<iostream>
#include"pool/threadpool.hpp"
#include"log/logger.h"
using namespace std;

void test_func(int a, int b) {
    printf("Task running, id: %d\n", a);
    LOG_DEBUG("Task running, id: %d", a);
}

int main() {
    webserver::Log::instance().init(webserver::Log::LogLevel::WARN, true);
    webserver::threadpool pool(8);

    pool.enqueue(test_func, 12, 34);
    for (int i = 0; i < 100; i++) {
        pool.enqueue(test_func, i, i);
    }
    LOG_FATAL("Hello, World!");
    getchar();
}