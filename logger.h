#ifndef LOGGER_H
#define LOGGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger {
private:
    std::shared_ptr<spdlog::logger> logger_;
    std::string app_name_;

    Logger(const std::string& name, const std::string& filename = "", bool console = true);

    // static map - storing all loggers
    static std::unordered_map<std::string, std::shared_ptr<Logger>> instances_;

public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // static method - getting logger
    static Logger& get_instance(const std::string& name,
        const std::string& filename = "",
        bool console = true);

    ~Logger();

    // basic methods
    template<typename... Args>
    void trace(const std::string& fmt, Args&&... args) {
        if (logger_) logger_->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const std::string& fmt, Args&&... args) {
        if (logger_) logger_->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        if (logger_) logger_->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const std::string& fmt, Args&&... args) {
        if (logger_) logger_->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        if (logger_) logger_->error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(const std::string& fmt, Args&&... args) {
        if (logger_) logger_->critical(fmt, std::forward<Args>(args)...);
    }

    // network events
    template<typename... Args>
    void network_in(const std::string& data, Args&&... args) {
        if (logger_) logger_->debug("[NET] <- {}", data, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void network_out(const std::string& data, Args&&... args) {
        if (logger_) logger_->debug("[NET] -> {}", data, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void network_error(const std::string& error, Args&&... args) {
        if (logger_) logger_->error("[NET] ERROR: {}", error, std::forward<Args>(args)...);
    }

    // authorization
    template<typename... Args>
    void auth_success(int user_id, bool is_admin, Args&&... args) {
        std::string role = is_admin ? "ADMIN" : "USER_" + std::to_string(user_id);
        if (logger_) logger_->info("[AUTH] SUCCESS for {}", role, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void auth_failed(int user_id, bool is_admin, const std::string& reason, Args&&... args) {
        std::string role = is_admin ? "ADMIN" : "USER_" + std::to_string(user_id);
        if (logger_) logger_->warn("[AUTH] FAILED for {}: {}", role, reason, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void auth_attempt(int user_id, bool is_admin, const std::string& action, Args&&... args) {
        std::string role = is_admin ? "ADMIN" : "USER_" + std::to_string(user_id);
        if (logger_) logger_->info("[AUTH] {} for {}", action, role, std::forward<Args>(args)...);
    }

    // comands
    template<typename... Args>
    void command_received(const std::string& cmd, const std::string& user_info = "", Args&&... args) {
        std::string log_msg = "[CMD] Received: {}";
        if (!user_info.empty()) log_msg += " from " + user_info;
        if (logger_) logger_->info(log_msg, cmd, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void command_success(const std::string& cmd, const std::string& details = "", Args&&... args) {
        if (details.empty()) {
            if (logger_) logger_->info("[CMD] {}: SUCCESS", cmd, std::forward<Args>(args)...);
        }
        else {
            if (logger_) logger_->info("[CMD] {}: SUCCESS - {}", cmd, details, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void command_failed(const std::string& cmd, const std::string& reason, Args&&... args) {
        if (logger_) logger_->warn("[CMD] {}: FAILED - {}", cmd, reason, std::forward<Args>(args)...);
    }

    // data
    template<typename... Args>
    void data_loaded(const std::string& entity, int count, Args&&... args) {
        if (logger_) logger_->debug("[DATA] Loaded {} {}", count, entity, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void data_saved(const std::string& entity, int count, Args&&... args) {
        if (logger_) logger_->debug("[DATA] Saved {} {}", count, entity, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void data_modified(const std::string& operation, const std::string& entity,
        const std::string& details = "", Args&&... args) {
        if (details.empty()) {
            if (logger_) logger_->info("[DATA] {} {}", operation, entity, std::forward<Args>(args)...);
        }
        else {
            if (logger_) logger_->info("[DATA] {} {} - {}", operation, entity, details, std::forward<Args>(args)...);
        }
    }

    // sockets
    template<typename... Args>
    void socket_connected(const std::string& endpoint, Args&&... args) {
        if (logger_) logger_->info("[SOCKET] Connected to {}", endpoint, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void socket_disconnected(const std::string& endpoint, Args&&... args) {
        if (logger_) logger_->info("[SOCKET] Disconnected from {}", endpoint, std::forward<Args>(args)...);
    }

    // orders
    template<typename... Args>
    void order_created(int order_id, int user_id, double total, Args&&... args) {
        if (logger_) logger_->info("[ORDER] Created: ID={}, User={}, Total={} rub.",
            order_id, user_id, total, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void order_updated(int order_id, const std::string& field,
        const std::string& old_value, const std::string& new_value, Args&&... args) {
        if (logger_) logger_->info("[ORDER] Updated: ID={}, {} changed from '{}' to '{}'",
            order_id, field, old_value, new_value, std::forward<Args>(args)...);
    }

    // users
    template<typename... Args>
    void user_created(int user_id, double initial_balance = 0.0, Args&&... args) {
        if (logger_) logger_->info("[USER] Created: ID={}, Balance={} rub.",
            user_id, initial_balance, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void user_balance_changed(int user_id, double old_balance, double new_balance, Args&&... args) {
        if (logger_) logger_->info("[USER] Balance changed: ID={}, {} -> {} rub.",
            user_id, old_balance, new_balance, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void user_deleted(int user_id, Args&&... args) {
        if (logger_) logger_->info("[USER] Deleted: ID={}", user_id, std::forward<Args>(args)...);
    }

    // menu
    template<typename... Args>
    void dish_added(const std::string& dish_name, double price, Args&&... args) {
        if (logger_) logger_->info("[MENU] Dish added: {} ({} rub.)", dish_name, price, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void dish_removed(const std::string& dish_name, Args&&... args) {
        if (logger_) logger_->info("[MENU] Dish removed: {}", dish_name, std::forward<Args>(args)...);
    }

    void set_level(spdlog::level::level_enum level);
    std::string get_app_name() const;
    bool is_initialized() const;
    std::shared_ptr<spdlog::logger> get_raw_logger();
};


inline Logger& get_server_logger() {
    return Logger::get_instance("SERVER_MAIN", "logs/server.log", true);
}

inline Logger& get_client_logger() {
    return Logger::get_instance("CLIENT", "logs/client.log", false);
}

inline Logger& get_database_logger() {
    return Logger::get_instance("DATABASE", "logs/database.log", false);
}

inline Logger& get_app_logger() {
    return Logger::get_instance("APP", "logs/app.log", false);
}

#endif 
