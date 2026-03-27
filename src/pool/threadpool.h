#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP
#include <thread>
#include <functional>
#include <future>
#include <type_traits>
#include "base/lock_free_queue.hpp"

namespace webserver {
    class threadpool {
    public:
        explicit threadpool(size_t thread_num, size_t capacity = 8192);
        ~threadpool();

        template <typename F, typename ...Args>
        auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    private:
        size_t capacity;

        std::vector<std::thread> workers;
        LockFreeQueue<std::function<void()>> task_queue;
        std::atomic<bool> stop{};
    };

    inline threadpool::threadpool(size_t thread_num, size_t capacity) {
        this->capacity = capacity;
        this->workers.reserve(thread_num);
        this->stop.store(false, std::memory_order_release);

        for (size_t i = 0; i < thread_num; i++) {
            workers.emplace_back([this]() {
                std::function<void()> task;

                while (stop.load(std::memory_order_acquire) == false) {
                    {
                        if (this->task_queue.dequeue(task)) {
                            task();
                        }
                        else {
                            if (stop.load(std::memory_order_acquire) == true) {
                                return;
                            }
                            std::this_thread::yield();
                        }
                    }
                }
            });
        }
    }

    inline threadpool::~threadpool() {
        this->stop.store(true, std::memory_order_release);
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template<typename F, typename... Args>
    auto threadpool::enqueue(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> res = task->get_future();

        if (stop.load(std::memory_order_acquire)) {
            throw std::runtime_error("enqueue on stopped threadpool");
        }
        if (task_queue.enqueue([=](){(*task)();}) == false) {
            throw std::runtime_error("task queue is full");
        }
        return res;
    }

}

#endif