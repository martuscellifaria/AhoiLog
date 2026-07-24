#include "VHLog.h"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <iostream>
#include <mutex>
#include <format>
#include <print>
#include <string>

VHLog::VHLog(bool debug_environment, std::size_t batch_size) : 
    base_path_and_name_("") {
    worker_running_ = true;
    batch_size_ = batch_size;
    unflushed_bytes_ = 0;
    debug_environment_ = debug_environment;
    sink_types_.clear();
    logger_thread_ = std::thread(&VHLog::logger_worker, this);
}

void VHLog::shutdown() {

    worker_running_ = false;
    
    cond_var_.notify_all();
    
    if (logger_thread_.joinable()) {
        logger_thread_.join();
    }

    if (file_ && file_.is_open()) {
        file_.flush();
        file_.close();
    }
    vhlog_shutdown_ = true;
}

VHLog::~VHLog() {

    if (!vhlog_shutdown_) {
        shutdown();
    }
}

void VHLog::logger_worker() {

    std::vector<std::pair<VHLogLevel, std::string>> batch;
    
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        cond_var_.wait(lock, [this]() {
            return !log_message_queue_.empty() || !worker_running_;
        });
        
        if (!worker_running_ && log_message_queue_.empty()) {
            break;
        }
        
        const size_t available = log_message_queue_.size();
        const size_t batch_size = std::min(available, static_cast<size_t>(batch_size_));
        
        batch.reserve(batch_size);
        
        for (size_t i = 0; i < batch_size; ++i) {
            batch.emplace_back(std::move(log_message_queue_.front()));
            log_message_queue_.pop_front();
        }
        
        lock.unlock(); 
        
        for (auto& [level, message] : batch) {
            if (!message.empty()) { 
                write_to_destination(level, message);
            }
        }
        
        batch.clear();
    }
    
    std::unique_lock<std::mutex> lock(queue_mutex_);
    while (!log_message_queue_.empty()) {
        auto& [level, message] = log_message_queue_.front();
        if (!message.empty()) {
            write_to_destination(level, message);
        }
        log_message_queue_.pop_front();
    }
}

void VHLog::add_console_sink() {
    
    std::lock_guard<std::mutex> lock(mutex_);
    append_new_sink(VHLogSinkType::ConsoleSink);
}


void VHLog::add_file_sink(const std::string& base_path_and_name, std::size_t max_size) {

    std::lock_guard<std::mutex> lock(mutex_);
    append_new_sink(VHLogSinkType::FileSink);
    
    if (base_path_and_name_ == "") {
        base_path_and_name_ = base_path_and_name;
    }
    
    if (file_ && file_.is_open()) {
        file_.close();
    }
    
    max_size_ = max_size;
    current_size_ = 0;
    
    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::floor<std::chrono::seconds>(now);
    auto zt = std::chrono::zoned_time(std::chrono::current_zone(), now_sec);
    current_date_ = std::format("{:%Y-%m-%d}", zt);
    std::string file_name = std::format("{}_{:%Y-%m-%d_%H-%M:%S}.log", base_path_and_name_, zt);


    file_.open(file_name, std::ios::app);
    if (!file_) {
        std::println("Failed to open/create log file: {}", file_name);
    }
}

void VHLog::rotate_file_sink() {
   
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (file_ && file_.is_open()) {
        file_.flush();
        file_.close();
    }

    current_size_ = 0;
    unflushed_bytes_ = 0;

    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::floor<std::chrono::seconds>(now);
    auto zt = std::chrono::zoned_time(std::chrono::current_zone(), now_sec);
    current_date_ = std::format("{:%Y-%m-%d}", zt);
    std::string file_name = std::format("{}_{:%Y-%m-%d_%H-%M:%S}.log", base_path_and_name_, zt);

    file_.open(file_name, std::ios::app);
    if (!file_) {
        std::println("Failed to open/create log file: {}", file_name);
    }
}

void VHLog::add_null_sink() {
    
    std::lock_guard<std::mutex> lock(mutex_);
    append_new_sink(VHLogSinkType::NullSink);
}

void VHLog::log(VHLogLevel level, std::string_view message) {
    
    if (level != VHLogLevel::DEBUG || debug_environment_) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        log_message_queue_.emplace_back(level, std::string(message));
        cond_var_.notify_one();
    }
}

void VHLog::write_to_destination(VHLogLevel level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::floor<std::chrono::seconds>(now);
    auto zt = std::chrono::zoned_time(std::chrono::current_zone(), now_sec);

    static constexpr const char* levels[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "UNKNOWN"
    };

    const char* level_string = levels[std::min(static_cast<int>(level), 5)];
    std::string composed_message = std::format("[{:%Y-%m-%d_%H-%M:%S}] [{}] {}\n",
                                             zt, 
                                             level_string, 
                                             message);
    
    for (const auto& sink_type : sink_types_) {
        switch (sink_type) {
            case VHLogSinkType::FileSink:
                {
                    if (file_ && file_.is_open()) {
                        file_ << composed_message;
                        current_size_ += composed_message.size();
                        unflushed_bytes_ += composed_message.size();
                        bool b_should_flush = false;
                        if (unflushed_bytes_ >= FLUSH_THRESHOLD) {
                            b_should_flush = true;
                        }
                        else if (level == VHLogLevel::FATAL || level == VHLogLevel::ERROR) {
                            b_should_flush = true;
                        }
                        else if (should_rotate(composed_message.size())) {
                            b_should_flush = true;
                            rotate_file_sink();
                        }
                        if (b_should_flush) {
                            file_.flush();
                            unflushed_bytes_ = 0;
                        }
                    }
                }
                break;
            case VHLogSinkType::ConsoleSink:
                std::print("{}", composed_message);
                break;
            case VHLogSinkType::NullSink:
                break;
        }
    }
}

bool VHLog::should_rotate(std::size_t message_size) {
    if (current_size_ + message_size > max_size_) {
        return true;
    }
    auto now = std::chrono::system_clock::now();
    std::string current_date = std::format("{:%Y-%m-%d}", now);
    
    if (current_date != current_date_) {
        return true;
    }
    return false;
}
