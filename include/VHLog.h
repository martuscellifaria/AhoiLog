#pragma once
#include <cstddef>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <memory>
#include <deque>
#include <thread>
#include <set>
#include <condition_variable>
#include <utility>
#include <type_traits>

enum class VHLogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class VHLog {
public:
    explicit VHLog(bool debug_environment = true, std::size_t batch_size = 1);
    void shutdown();
    virtual ~VHLog();

    static std::shared_ptr<VHLog> instance() {
        static auto vh_log = std::shared_ptr<VHLog>(new VHLog);
        return vh_log;
    }

private:
    enum class VHLogSinkType {
        ConsoleSink,
        FileSink,
        NullSink,
    };

public:
    void add_console_sink();
    void add_file_sink(const std::string& base_path_and_name = "", std::size_t max_size = 1024*1024);
    void add_null_sink();

    void log(VHLogLevel level, const std::string_view message);

private:
    void write_to_destination(VHLogLevel level, const std::string& message);
    void append_new_sink(VHLogSinkType new_sink) { sink_types_.insert(new_sink); }
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

    std::deque<std::pair<VHLogLevel, std::string>> log_message_queue_;
    bool debug_environment_;
    std::mutex queue_mutex_;
    std::condition_variable cond_var_;
    std::string base_path_and_name_;
    std::size_t max_size_;
    std::size_t current_size_;
    std::string current_date_;
    std::set<VHLogSinkType> sink_types_;
    static constexpr std::size_t FLUSH_THRESHOLD = 4096;
    bool vhlog_shutdown_;
};
