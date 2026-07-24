#pragma once
#include <cstddef>
#include <format>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <chrono>
#include <deque>
#include <thread>
#include <set>
#include <source_location>
#include <condition_variable>
#include <utility>

enum class AhoiLogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class AhoiLog {
public:
    explicit AhoiLog(bool debug_environment = true, std::size_t batch_size = 1);
    virtual ~AhoiLog() noexcept;
    AhoiLog(const AhoiLog&) = delete;
    AhoiLog& operator=(const AhoiLog&) = delete;
    void shutdown();
private:
    enum class AhoiLogSinkType {
        ConsoleSink,
        FileSink,
        NullSink,
    };
public:
    void add_console_sink();
    void add_file_sink(const std::string& base_path_and_name = "", std::size_t max_size = 1024*1024);
    void add_null_sink();

    template<typename... Args>
    void log(AhoiLogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (ahoilog_shutdown_) return;
        if (level != AhoiLogLevel::DEBUG || debug_environment_) {
            auto formatted = std::format(fmt, std::forward<Args>(args)...);
            std::lock_guard<std::mutex> lock(queue_mutex_);
            log_message_queue_.emplace_back(level, std::move(formatted));
            cond_var_.notify_one();
        }
    }

    void log(AhoiLogLevel level, const std::string_view message);
    static std::string where_am_i(std::source_location loc = std::source_location::current()) {
        return std::format("[{}:{}]", loc.file_name(), loc.line());
    }

private:
    void write_to_destination(AhoiLogLevel level, const std::string& message);
    void append_new_sink(AhoiLogSinkType new_sink) { sink_types_.insert(new_sink); }
    bool should_rotate(std::size_t message_size);
    void rotate_file_sink();

    std::mutex mutex_;
    std::mutex file_mutex_;
    std::ofstream file_;
    std::size_t unflushed_bytes_;
    std::thread logger_thread_;
    void logger_worker();
    bool worker_running_;
    std::size_t batch_size_;

    std::deque<std::pair<AhoiLogLevel, std::string>> log_message_queue_;
    bool debug_environment_;
    std::mutex queue_mutex_;
    std::condition_variable cond_var_;
    std::string base_path_and_name_;
    std::size_t max_size_;
    std::size_t current_size_;
    std::chrono::year_month_day current_date_;
    std::string cached_timestamp_;
    const std::string& get_timestamp();
    std::chrono::year_month_day get_current_date() const;
    std::chrono::sys_seconds last_timestamp_sec_;
    std::set<AhoiLogSinkType> sink_types_;
    static constexpr std::size_t FLUSH_THRESHOLD = 4096;
    bool ahoilog_shutdown_;
};
