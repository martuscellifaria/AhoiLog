#include "AhoiLog.h"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <exception>
#include <iostream>
#include <format>
#include <print>
#include <string>

AhoiLog::AhoiLog() : 
    base_path_and_name_(""),
    ahoilog_shutdown_(false) {
    worker_running_ = true;
    unflushed_bytes_ = 0;
    sink_types_.clear();
    logger_thread_ = std::thread(&AhoiLog::logger_worker, this);
}

void AhoiLog::shutdown() {
    ahoilog_shutdown_ = true;
    worker_running_ = false;

    if (logger_thread_.joinable()) {
        logger_thread_.join();
    }

    if (file_ && file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

AhoiLog::~AhoiLog() noexcept {
    try {
        if (!ahoilog_shutdown_) {
            shutdown();
        }
    }
    catch (...) {
        std::terminate();
    }
}

void AhoiLog::logger_worker() {
    while (true) {
        size_t local_read = read_pos_.load(std::memory_order_relaxed);
        size_t local_write = write_pos_.load(std::memory_order_acquire);
        
        while (local_read != local_write) {
            auto& msg = log_message_queue_[local_read & QUEUE_MASK];
            std::string message;
	    if (msg.is_large) {
                message.assign(msg.large, msg.length);
                delete[] msg.large;
            }
	    else {
                message.assign(msg.small, msg.length);
            }
            
            if (!message.empty()) {
                write_to_destination(msg.level, message);
            }
            
            local_read++;
        }
        
        read_pos_.store(local_read, std::memory_order_release);
        
	if (!worker_running_) {
            local_write = write_pos_.load(std::memory_order_acquire);
            if (local_read == local_write) {
		break;
	    }
        }
	else {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
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
    
    if (base_path_and_name_ == "") {
        base_path_and_name_ = base_path_and_name;
    }
    
    if (file_ && file_.is_open()) {
        file_.close();
    }
    
    max_size_ = max_size;
    current_size_ = 0;
    
    current_date_ = get_current_date();

    auto now = std::chrono::zoned_time(
        std::chrono::current_zone(),
        std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    std::string file_name = std::format("{}_{:%Y-%m-%d_%H-%M:%S}.log", base_path_and_name_, now);

    file_.open(file_name, std::ios::app);
    if (!file_) {
        std::println("Failed to open/create log file: {}", file_name);
    }
}

void AhoiLog::rotate_file_sink() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (file_ && file_.is_open()) {
        file_.flush();
        file_.close();
    }

    current_size_ = 0;
    unflushed_bytes_ = 0;

    current_date_ = get_current_date();
    auto now = std::chrono::zoned_time(
        std::chrono::current_zone(),
        std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    std::string file_name = std::format("{}_{:%Y-%m-%d_%H-%M:%S}.log", base_path_and_name_, now);
    file_.open(file_name, std::ios::app);
    if (!file_) {
        std::println("Failed to open/create log file: {}", file_name);
    }
}

void AhoiLog::add_null_sink() {
    std::lock_guard<std::mutex> lock(mutex_);
    append_new_sink(AhoiLogSinkType::NullSink);
}

void AhoiLog::log(AhoiLogLevel level, std::string_view message) {
    if (ahoilog_shutdown_) return;
    if constexpr (!DEBUG_ENABLED) {
	if (level == AhoiLogLevel::DEBUG) return;
    }
    size_t pos = write_pos_.fetch_add(1, std::memory_order_relaxed) & QUEUE_MASK;
    auto& msg = log_message_queue_[pos];
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
}

void AhoiLog::write_to_destination(AhoiLogLevel level, const std::string& message) {
    static constexpr const char* levels[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "UNKNOWN"
    };

    const char* level_string = levels[std::min(static_cast<int>(level), 5)];
    std::string composed_message = std::format("[{}] [{}] {}\n", get_timestamp(), 
            level_string, 
            message);
    for (const auto& sink_type : sink_types_) {
        switch (sink_type) {
            case AhoiLogSinkType::FileSink:
                {
                    if (file_ && file_.is_open()) {
                        file_ << composed_message;
                        current_size_ += composed_message.size();
                        unflushed_bytes_ += composed_message.size();
                        bool should_flush = false;
                        if (unflushed_bytes_ >= FLUSH_THRESHOLD) {
                            should_flush = true;
                        }
                        else if (level == AhoiLogLevel::FATAL || 
                                level == AhoiLogLevel::ERROR || 
                                level == AhoiLogLevel::WARNING) {
                            should_flush = true;
                        }
                        else if (should_rotate(composed_message.size())) {
                            should_flush = true;
                            rotate_file_sink();
                        }
                        if (should_flush) {
                            file_.flush();
                            unflushed_bytes_ = 0;
                        }
                    }
                }
                break;
            case AhoiLogSinkType::ConsoleSink:
                std::print("{}", composed_message);
                break;
            case AhoiLogSinkType::NullSink:
                break;
        }
    }
}

bool AhoiLog::should_rotate(std::size_t message_size) {
    if (current_size_ + message_size > max_size_) {
        return true;
    }
    return get_current_date() != current_date_;
}

const std::string& AhoiLog::get_timestamp() {
    auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
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
