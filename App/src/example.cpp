#include <print>
#include <string>
#include "version.h"
#include "AhoiLog.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/file.h>
    #include <unistd.h>
    #include <cerrno>
#endif

extern const char* embedded_version;

int main() {
    AhoiLog ahoi_log = AhoiLog();
    ahoi_log.add_console_sink();
    ahoi_log.add_file_sink("AhoiLogTest", 1024*1024);
    ahoi_log.log(AhoiLogLevel::DEBUG, "AhoiLog Start");
    ahoi_log.log(AhoiLogLevel::INFO, "AhoiLog is an async logging library created by AhoiLabs using AhoiCpp. {}", ahoi_log.where_am_i());
    ahoi_log.log(AhoiLogLevel::DEBUG, "AhoiLog End");
    ahoi_log.shutdown();
    return 0;
}
