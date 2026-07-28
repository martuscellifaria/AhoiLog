#include "AhoiLog.h"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <exception>
#include <iostream>
#include <format>
#include <print>
#include <string>
#include <cstdlib>

AhoiLog::AhoiLog() 
    : base_path_and_name_(""),
    max_size_(1024 * 1024),
    current_size_(0),
    unflushed_bytes_(0),
    sink_mask_(0) {
    const char* env_debug = std::getenv("AHOI_LOG_DEBUG");
    debug_environment_ = (env_debug != nullptr && std::string_view(env_debug) != "0");
    
    logger_thread_ = std::jthread([this](std::stop_token st) {
        logger_worker(st);
    });
}

AhoiLog::~AhoiLog() noexcept {
    try {
        if (!ahoilog_shutdown_.load(std::memory_order_acquire)) {
            shutdown();
        }
    }
    catch (...) {
        std::terminate();
    }
}

void AhoiLog::shutdown() {
    ahoilog_shutdown_.store(true, std::memory_order_release);
    logger_thread_.request_stop();
    queue_cv_.notify_all();
    
    if (logger_thread_.joinable()) {
        logger_thread_.join();
    }
    
    std::lock_guard<std::mutex> file_lock(file_mutex_);
    if (file_ && file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void AhoiLog::logger_worker(std::stop_token st) {
    while (!st.stop_requested()) {
        std::deque<LogMessage> batch;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [&] {
                return !log_message_queue_.empty() || st.stop_requested();
            });
            
            if (log_message_queue_.empty() && st.stop_requested()) {
                break;
            }
            
            batch = std::move(log_message_queue_);
            log_message_queue_.clear();
        }
        
        for (auto& msg : batch) {
            std::string message;
            if (msg.is_large) {
                message.assign(msg.large, msg.length);
            }
            else {
                message.assign(msg.small, msg.length);
            }
            
            if (!message.empty()) {
                write_to_destination(msg.level, message);
            }
        }
    }
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        while (!log_message_queue_.empty()) {
            auto msg = std::move(log_message_queue_.front());
            log_message_queue_.pop_front();
            std::string message;
            if (msg.is_large) {
                message.assign(msg.large, msg.length);
            }
            else {
                message.assign(msg.small, msg.length);
            }
            auto level = msg.level;
            lock.unlock();
            if (!message.empty()) {
                write_to_destination(level, message);
            }
            lock.lock();
        }
    }
}

void AhoiLog::add_console_sink() {
    std::lock_guard<std::mutex> lock(mutex_);
    append_new_sink(AhoiLogSinkType::ConsoleSink);
}

void AhoiLog::add_file_sink(const std::string& base_path_and_name, std::size_t max_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    append_new_sink(AhoiLogSinkType::FileSink);
    
    if (base_path_and_name_.empty()) {
        base_path_and_name_ = base_path_and_name;
    }
    
    {
        std::lock_guard<std::mutex> file_lock(file_mutex_);
        if (file_ && file_.is_open()) {
            file_.close();
        }
        
        max_size_ = max_size;
        current_size_ = 0;
        
        current_date_ = get_current_date();
        
        auto now = std::chrono::zoned_time(
            std::chrono::current_zone(),
            std::chrono::floor<std::chrono::minutes>(std::chrono::system_clock::now()));
        std::string file_name = std::format("{}_{:%Y-%m-%d_%H-%M}.log", base_path_and_name_, now);
        file_.open(file_name, std::ios::app);
        if (!file_) {
            std::println("Failed to open/create log file: {}", file_name);
        }
    }
}

void AhoiLog::add_null_sink() {
    std::lock_guard<std::mutex> lock(mutex_);
    append_new_sink(AhoiLogSinkType::NullSink);
}

void AhoiLog::set_log_options(AhoiLogSinkType sink_type,
                               const std::string& base_path_and_name,
                               std::size_t max_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    sink_mask_ = 0;
    append_new_sink(sink_type);
    
    if (static_cast<uint8_t>(sink_type) & static_cast<uint8_t>(AhoiLogSinkType::FileSink)) {
        base_path_and_name_ = base_path_and_name;
        max_size_ = max_size;
        
        std::lock_guard<std::mutex> file_lock(file_mutex_);
        if (file_ && file_.is_open()) {
            file_.close();
        }
        
        current_size_ = 0;
        current_date_ = get_current_date();
        
        auto now = std::chrono::zoned_time(
            std::chrono::current_zone(),
            std::chrono::floor<std::chrono::minutes>(std::chrono::system_clock::now()));
        std::string file_name = std::format("{}_{:%Y-%m-%d_%H-%M}.log", base_path_and_name_, now);
        file_.open(file_name, std::ios::app);
        if (!file_) {
            std::println("Failed to open/create log file: {}", file_name);
        }
    }
}

void AhoiLog::log(AhoiLogLevel level, std::string_view message) {
    if (ahoilog_shutdown_.load(std::memory_order_acquire)) return;
    
    if constexpr (!DEBUG_ENABLED) {
        if (level == AhoiLogLevel::DEBUG) return;
    }
    
    LogMessage msg;
    msg.level = level;
    
    if (message.size() < sizeof(msg.small) - 1) {
        std::memcpy(msg.small, message.data(), message.size());
        msg.small[message.size()] = '\0';
        msg.length = message.size();
        msg.is_large = false;
    }
    else {
        msg.large = new char[message.size() + 1];
        std::memcpy(msg.large, message.data(), message.size());
        msg.large[message.size()] = '\0';
        msg.length = message.size();
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

void AhoiLog::write_to_destination(AhoiLogLevel level, const std::string& message) {
    static constexpr const char* levels[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
    };
    
    const char* level_string = (static_cast<int>(level) < 5) 
        ? levels[static_cast<int>(level)] 
        : "UNKNOWN";
    
    std::string composed_message = std::format(
        "[{}] [{}] {}\n", 
        get_timestamp(), 
        level_string, 
        message
    );
    
    uint8_t sink_mask;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sink_mask = sink_mask_;
    }
    
    bool has_console = (sink_mask & static_cast<uint8_t>(AhoiLogSinkType::ConsoleSink)) != 0;
    bool has_file = (sink_mask & static_cast<uint8_t>(AhoiLogSinkType::FileSink)) != 0;
    bool has_null = (sink_mask & static_cast<uint8_t>(AhoiLogSinkType::NullSink)) != 0;
    
    if (has_null && !has_console && !has_file) {
        return;
    }
    
    if (has_file) {
        std::lock_guard<std::mutex> file_lock(file_mutex_);
        
        if (!file_ || !file_.is_open()) {
            if (!base_path_and_name_.empty()) {
                current_date_ = get_current_date();
                auto now = std::chrono::zoned_time(
                    std::chrono::current_zone(),
                    std::chrono::floor<std::chrono::minutes>(std::chrono::system_clock::now()));
                std::string file_name = std::format("{}_{:%Y-%m-%d_%H-%M}.log", base_path_and_name_, now);
                file_.open(file_name, std::ios::app);
            }
        }
        if (file_ && file_.is_open()) {
            file_ << composed_message;
            current_size_ += composed_message.size();
            unflushed_bytes_ += composed_message.size();
            bool should_flush = false;
            if (unflushed_bytes_ >= FLUSH_THRESHOLD) {
                should_flush = true;
            }
            else if (level == AhoiLogLevel::CRITICAL || 
                     level == AhoiLogLevel::ERROR || 
                     level == AhoiLogLevel::WARNING) {
                should_flush = true;
            }
            else if (should_rotate(composed_message.size())) {
                should_flush = true;
                rotate_file_sink_internal();
            }
            if (should_flush) {
                file_.flush();
                unflushed_bytes_ = 0;
            }
        }
    }
    if (has_console) {
        std::print("{}", composed_message);
    }
}

bool AhoiLog::should_rotate(std::size_t message_size) {
    if (current_size_ + message_size > max_size_) {
        return true;
    }
    return get_current_date() != current_date_;
}

void AhoiLog::rotate_file_sink() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    rotate_file_sink_internal();
}

void AhoiLog::rotate_file_sink_internal() {
    if (file_ && file_.is_open()) {
        file_.flush();
        file_.close();
    }
    current_size_ = 0;
    unflushed_bytes_ = 0;
    current_date_ = get_current_date();
    auto now = std::chrono::zoned_time(
        std::chrono::current_zone(),
        std::chrono::floor<std::chrono::minutes>(std::chrono::system_clock::now()));
    std::string file_name = std::format("{}_{:%Y-%m-%d_%H-%M}.log", base_path_and_name_, now);
    file_.open(file_name, std::ios::app);
    if (!file_) {
        std::println("Failed to open/create log file: {}", file_name);
    }
}

const std::string& AhoiLog::get_timestamp() {
    auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    std::lock_guard<std::mutex> lock(timestamp_mutex_);
    if (now != last_timestamp_sec_) {
        last_timestamp_sec_ = now;
        cached_timestamp_ = std::format("{:%Y-%m-%d_%H-%M:%S}", 
            std::chrono::zoned_time(std::chrono::current_zone(), now));
    }
    return cached_timestamp_;
}

std::chrono::year_month_day AhoiLog::get_current_date() const {
    auto now = std::chrono::floor<std::chrono::days>(
        std::chrono::zoned_time(
            std::chrono::current_zone(),
            std::chrono::system_clock::now()
        ).get_local_time()
    );
    return std::chrono::year_month_day{now};
}
