#ifndef WEBSERVER_LOGGER_H
#define WEBSERVER_LOGGER_H

#include <mutex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <memory>
#include <atomic>
#include <iomanip>
#include <iostream>
#include "util/lock_free_queue.hpp"

namespace webserver {
    namespace fs = std::filesystem;

    #define COLOR_DEBUG  "\033[36m"  // 青色
    #define COLOR_INFO   "\033[32m"  // 绿色
    #define COLOR_WARN   "\033[33m"  // 黄色
    #define COLOR_ERROR  "\033[31m"  // 红色
    #define COLOR_FATAL  "\033[1;31m"// 加粗红
    #define COLOR_RESET  "\033[0m"

    class Log {
    public:
        enum class LogLevel : int { DEBUG = 0, INFO, WARN, ERROR, FATAL };

        static Log& instance() {
            static Log instance;
            return instance;
        }

        Log(const Log&) = delete;
        Log& operator=(const Log&) = delete;

        void init(LogLevel level = LogLevel::DEBUG, bool is_async = false, const fs::path& path = "./log",
                  std::string_view suffix = ".log");

        template<typename... Args>
        void write(LogLevel level, const char* file, int line, const char* format, Args&&... args) {
            if (!is_open_ || level < log_level_) return;

            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            struct tm tm_info;
#ifdef _WIN32
            localtime_s(&tm_info, &t);
#else
            localtime_r(&t, &tm_info);
#endif

            char content[1024];
            // 建议调用时 LOG_DEBUG("%s", str.c_str())
            int len = snprintf(content, sizeof(content), format, std::forward<Args>(args)...);

            // 3. 组合整行日志
            if (len > 0 && content[len - 1] == '\n') content[len - 1] = '\0';
            std::ostringstream oss;
            oss << std::put_time(&tm_info, "%Y-%m-%d %H:%M:%S")
                << " " << level_to_string(level)
                << " [" << file << ":" << line << "] "
                << content << "\n";

            std::string log_entry = oss.str();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                std::cout << get_color_code(level) << log_entry << COLOR_RESET;
            }
                if (!is_open_) return;

            if (is_async_ && log_queue_) {
                log_queue_->enqueue(std::move(log_entry));
            } else {
                std::lock_guard<std::mutex> lock(mutex_);
                if (log_file_.is_open()) {
                    log_file_ << log_entry;
                    log_file_.flush();
                }
            }
        }

        void flush();
        void set_log_level(LogLevel level) { log_level_ = level; }
        LogLevel get_log_level() const { return log_level_; }

    private:
        Log() = default;
        ~Log();

        static const char* level_to_string(LogLevel level) {
            switch (level) {
                case LogLevel::DEBUG: return "[DEBUG]";
                case LogLevel::INFO:  return "[INFO ]";
                case LogLevel::WARN:  return "[WARN ]";
                case LogLevel::ERROR: return "[ERROR]";
                case LogLevel::FATAL: return "[FATAL]";
                default: return "[UNKNOWN]";
            }
        }

        static const char* get_color_code(LogLevel level) {
            switch (level) {
                case LogLevel::DEBUG: return COLOR_DEBUG;
                case LogLevel::INFO:  return COLOR_INFO;
                case LogLevel::WARN:  return COLOR_WARN;
                case LogLevel::ERROR: return COLOR_ERROR;
                case LogLevel::FATAL: return COLOR_FATAL;
                default: return COLOR_RESET;
            }
        }

        void async_write();

        LogLevel log_level_ = LogLevel::DEBUG;
        std::atomic<bool> is_async_{false};
        std::atomic<bool> is_open_{false};
        std::atomic<bool> stop_thread_{false};

        std::ofstream log_file_;
        std::unique_ptr<webserver::LockFreeQueue<std::string>> log_queue_;
        std::unique_ptr<std::thread> thread_;
        std::mutex mutex_;
    };

    #define LOG_BASE(level, format, ...) \
        webserver::Log::instance().write(level, __FILE__, __LINE__, format, ##__VA_ARGS__)

    #define LOG_DEBUG(format, ...) LOG_BASE(webserver::Log::LogLevel::DEBUG, format, ##__VA_ARGS__)
    #define LOG_INFO(format, ...)  LOG_BASE(webserver::Log::LogLevel::INFO, format, ##__VA_ARGS__)
    #define LOG_WARN(format, ...)  LOG_BASE(webserver::Log::LogLevel::WARN, format, ##__VA_ARGS__)
    #define LOG_ERROR(format, ...) LOG_BASE(webserver::Log::LogLevel::ERROR, format, ##__VA_ARGS__)
    #define LOG_FATAL(format, ...) LOG_BASE(webserver::Log::LogLevel::FATAL, format, ##__VA_ARGS__)
}
#endif
