#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace core {
    class Logger {
    private:
        static std::shared_ptr<spdlog::logger> logger_;

        static void ensureInitialized() {
            if (!logger_) {
                logger_ = spdlog::stdout_color_mt("ptz_commander");
                logger_->set_pattern("[%^%l%$] %v");
                logger_->set_level(spdlog::level::info);
            }
        }

    public:
        static void info(const std::string& message) {
            ensureInitialized();
            logger_->info(message);
        }

        static void warn(const std::string& message) {
            ensureInitialized();
            logger_->warn(message);
        }

        static void error(const std::string& message) {
            ensureInitialized();
            logger_->error(message);
        }

        static void debug(const std::string& message) {
            ensureInitialized();
            logger_->debug(message);
        }
    };

    // Static member definition
    inline std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;
}

#endif // CORE_LOGGER_H
