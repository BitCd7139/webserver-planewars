
#include "logger.h"


namespace webserver {

    Log::~Log() {
        stop_thread_ = true;
        if (thread_ && thread_->joinable()) {
            thread_->join(); // 等待异步线程退出
        }
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    void Log::init(LogLevel level, bool is_async, const fs::path& path, std::string_view suffix) {
        log_level_ = level;

        try {
            if (!fs::exists(path)) {
                fs::create_directories(path);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Warning: Could not create log directory: " << e.what() << "\n";
        }

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm tm_info;
#ifdef _WIN32
        localtime_s(&tm_info, &now);
#else
        localtime_r(&now, &tm_info);
#endif
        char fileName[128];
        snprintf(fileName, sizeof(fileName), "%04d_%02d_%02d%s",
                 tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday, suffix.data());

        {
            std::lock_guard<std::mutex> lock(mutex_);
            log_file_.open(path / fileName, std::ios::app);
            if (!log_file_.is_open()) {
                std::cerr << "Critical Error: Could not open log file!" << std::endl;
                return;
            }
        }

        if (is_async == true) {
            is_async_ = true;
            stop_thread_ = false;
            const int maxQueueSize = 1024;
            log_queue_ = std::make_unique<LockFreeQueue<std::string>>(maxQueueSize);
            thread_ = std::make_unique<std::thread>(&Log::async_write, this);
        }

        is_open_ = true;
    }

    void Log::flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open()) {
            log_file_.flush();
        }
    }

    void Log::async_write() {
        std::string log_message;
        while (true) {
            if (log_queue_->dequeue(log_message)) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (log_file_.is_open()) {
                    log_file_ << log_message;
                }
            } else {
                if (stop_thread_.load()) {
                    break;
                }
                std::this_thread::yield();
            }
        }

        while(log_queue_->dequeue(log_message)) {
            if (log_file_.is_open()) log_file_ << log_message;
        }
        if (log_file_.is_open()) log_file_.flush();
    }
}