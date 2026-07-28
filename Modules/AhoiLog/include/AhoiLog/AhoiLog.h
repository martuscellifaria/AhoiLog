#pragma once
#ifndef AHOI_LOG_DEBUG
#define AHOI_LOG_DEBUG 1
#endif

#include <cstddef>
#include <cstring>
#include <format>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <source_location>
#include <utility>

enum class AhoiLogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

enum class AhoiLogSinkType : uint8_t {
    ConsoleSink = 1 << 0,
    FileSink    = 1 << 1,
    NullSink    = 1 << 2,
};

class AhoiLog {
public:
    explicit AhoiLog();
    ~AhoiLog() noexcept;
    AhoiLog(const AhoiLog&) = delete;
    AhoiLog& operator=(const AhoiLog&) = delete;
    void shutdown();

private:
    struct LogMessage {
        AhoiLogLevel level;
        union {
            char small[256];
            char* large;
        };
        size_t length;
        bool is_large;

        LogMessage() : level(AhoiLogLevel::INFO), length(0), is_large(false) {
            small[0] = '\0';
        }
        
        ~LogMessage() {
            if (is_large) {
                delete[] large;
            }
        }
        
        LogMessage(LogMessage&& other) noexcept 
            : level(other.level), length(other.length), is_large(other.is_large) {
            if (other.is_large) {
                large = other.large;
                other.large = nullptr;
                other.is_large = false;
            }
            else {
                std::memcpy(small, other.small, length + 1);
                small[length] = '\0';
            }
            other.length = 0;
            other.is_large = false;
        }
        
        LogMessage& operator=(LogMessage&& other) noexcept {
            if (this != &other) {
                if (is_large) delete[] large;
                level = other.level;
                length = other.length;
                is_large = other.is_large;
                if (other.is_large) {
                    large = other.large;
                    other.large = nullptr;
                    other.is_large = false;
                } else {
                    std::memcpy(small, other.small, length + 1);
                }
                other.length = 0;
            }
            return *this;
        }
        
        LogMessage(const LogMessage&) = delete;
        LogMessage& operator=(const LogMessage&) = delete;
    };

public:
    void add_console_sink();
    void add_file_sink(const std::string& base_path_and_name = "", 
                       std::size_t max_size = 1024 * 1024);
    void add_null_sink();
    void set_log_options(AhoiLogSinkType sink_type,
                         const std::string& base_path_and_name = "",
                         std::size_t max_size = 1024 * 1024);

    template<typename... Args>
    void log(AhoiLogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (ahoilog_shutdown_.load(std::memory_order_acquire)) return;
        
        if constexpr (!DEBUG_ENABLED) {
            if (level == AhoiLogLevel::DEBUG) return;
        }
        
        auto formatted = std::format(fmt, std::forward<Args>(args)...);
        LogMessage msg;
        msg.level = level;
        
        if (formatted.size() < sizeof(msg.small) - 1) {
            std::memcpy(msg.small, formatted.data(), formatted.size());
            msg.small[formatted.size()] = '\0';
            msg.length = formatted.size();
            msg.is_large = false;
        }
        else {
            msg.large = new char[formatted.size() + 1];
            std::memcpy(msg.large, formatted.data(), formatted.size());
            msg.large[formatted.size()] = '\0';
            msg.length = formatted.size();
            msg.is_large = true;
        }
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            bool was_empty = log_message_queue_.empty();
            log_message_queue_.emplace_back(std::move(msg));
            if (was_empty) {
                queue_cv_.notify_one();
            }
        }
    }
    
    void log(AhoiLogLevel level, std::string_view message);
    
    static std::string where_am_i(std::source_location loc = std::source_location::current()) {
        return std::format("[{}:{}]", loc.file_name(), loc.line());
    }

private:
    void write_to_destination(AhoiLogLevel level, const std::string& message);
    void append_new_sink(AhoiLogSinkType new_sink) { 
        sink_mask_ |= static_cast<uint8_t>(new_sink); 
    }
    bool should_rotate(std::size_t message_size);
    void rotate_file_sink();
    void rotate_file_sink_internal();

    std::mutex mutex_;
    std::mutex file_mutex_;
    std::mutex timestamp_mutex_;
    std::ofstream file_;
    std::size_t unflushed_bytes_ = 0;
    std::jthread logger_thread_;
    void logger_worker(std::stop_token st);

    bool debug_environment_ = false;
    std::string base_path_and_name_;
    std::size_t max_size_ = 1024 * 1024;
    std::size_t current_size_ = 0;
    std::chrono::year_month_day current_date_;
    std::string cached_timestamp_;
    const std::string& get_timestamp();
    std::chrono::year_month_day get_current_date() const;
    std::chrono::sys_seconds last_timestamp_sec_;
    
    uint8_t sink_mask_ = 0;
    
    static constexpr std::size_t FLUSH_THRESHOLD = 4096;
    static constexpr bool DEBUG_ENABLED = AHOI_LOG_DEBUG;
    
    std::deque<LogMessage> log_message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> ahoilog_shutdown_{false};
};
