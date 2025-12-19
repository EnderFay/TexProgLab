#include "logger.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#else
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

// initializing static map
std::unordered_map<std::string, std::shared_ptr<Logger>> Logger::instances_;

Logger::Logger(const std::string& name, const std::string& filename, bool console)
    : app_name_(name) {

    try {
        // creating logs folder
        if (!fs::exists("logs")) {
            fs::create_directories("logs");
        }

        std::string log_filename;
        if (!filename.empty()) {
            log_filename = filename;
        }
        else {
            auto now = std::chrono::system_clock::now();
            auto now_c = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&now_c);
            std::ostringstream oss;
            oss << "logs/" << name << "_"
                << (tm.tm_year + 1900) << "-"
                << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1) << "-"
                << std::setw(2) << std::setfill('0') << tm.tm_mday
                << ".log";
            log_filename = oss.str();
        }

        std::vector<spdlog::sink_ptr> sinks;

        try {
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_filename, 10 * 1024 * 1024, 3);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
            sinks.push_back(file_sink);
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Failed to create file sink for " << name << ": " << ex.what() << std::endl;
            throw;
        }

        if (console) {
            try {
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                console_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] %n: %v");
                sinks.push_back(console_sink);
            }
            catch (const spdlog::spdlog_ex& ex) {
                std::cerr << "Failed to create console sink for " << name << ": " << ex.what() << std::endl;
            }
        }

        std::shared_ptr<spdlog::logger> existing_logger;
        try {
            existing_logger = spdlog::get(name);
        }
        catch (...) {
            existing_logger = nullptr;
        }

        if (existing_logger) {
            logger_ = existing_logger;
            std::cout << "[LOGGER] Reusing existing logger: " << name << std::endl;
        }
        else {
            // creating new logger
            try {
                logger_ = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
                spdlog::register_logger(logger_);
            }
            catch (const spdlog::spdlog_ex& ex) {
                std::cerr << "Failed to create logger " << name << ": " << ex.what() << std::endl;
                throw;
            }
        }

        // setting logging level
#ifdef NDEBUG
        logger_->set_level(spdlog::level::info);
#else
        logger_->set_level(spdlog::level::debug);
#endif

        logger_->flush_on(spdlog::level::err);

        // logging, if log new
        if (!existing_logger) {
            logger_->info("================================================");
            logger_->info("Logger '{}' initialized successfully", name);
            logger_->info("Log file: {}", log_filename);
            logger_->info("Log level: {}", spdlog::level::to_string_view(logger_->level()));
            logger_->info("Console output: {}", console ? "ENABLED" : "DISABLED");
            logger_->info("================================================");
        }

    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "SPDLOG ERROR for " << name << ": " << ex.what() << std::endl;

        try {
            // Fallback: creating a simple console logger
            logger_ = spdlog::stdout_color_mt(name);
            logger_->error("Failed to create file logger for {}, using console only. Error: {}",
                name, ex.what());
        }
        catch (...) {
            std::cerr << "CRITICAL: Cannot create even console logger for " << name << "!" << std::endl;
            throw;
        }

    }
    catch (const std::exception& ex) {
        std::cerr << "GENERAL ERROR for " << name << ": " << ex.what() << std::endl;

        try {
            logger_ = spdlog::stdout_color_mt(name);
            logger_->critical("Critical error for {}: {}", name, ex.what());
        }
        catch (...) {
            std::cerr << "CRITICAL: Cannot create any logger for " << name << "!" << std::endl;
            throw;
        }
    }
    catch (...) {
        std::cerr << "UNKNOWN ERROR for " << name << std::endl;

        try {
            logger_ = spdlog::stdout_color_mt(name);
            logger_->critical("Unknown critical error for {}", name);
        }
        catch (...) {
            std::cerr << "CRITICAL: Cannot create any logger for " << name << "!" << std::endl;
            throw;
        }
    }
}

Logger::~Logger() {
}

Logger& Logger::get_instance(const std::string& name,
    const std::string& filename,
    bool console) {
    auto it = instances_.find(name);
    if (it == instances_.end()) {
        // creating new logger
        auto logger = std::shared_ptr<Logger>(new Logger(name, filename, console));
        instances_[name] = logger;
        return *logger;
    }
    return *(it->second);
}

void Logger::set_level(spdlog::level::level_enum level) {
    if (logger_) {
        logger_->set_level(level);
        logger_->info("Log level changed to: {}", spdlog::level::to_string_view(level));
    }
}

std::string Logger::get_app_name() const {
    return app_name_;
}

bool Logger::is_initialized() const {
    return logger_ != nullptr;
}

std::shared_ptr<spdlog::logger> Logger::get_raw_logger() {
    return logger_;
}
