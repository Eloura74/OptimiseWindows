#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <vector>
#include <deque>
#include <sstream>

namespace lsaa {

    enum class LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERR
    };

    class Logger {
    public:
        static Logger& instance() {
            static Logger instance;
            return instance;
        }

        Logger(const Logger&) = delete;
        void operator=(const Logger&) = delete;

        void init(const std::string& filename) {
             std::lock_guard<std::mutex> lock(mutex_);
             if (fileStream_.is_open()) fileStream_.close();
             fileStream_.open(filename, std::ios::app);
        }

        void log(LogLevel level, const std::string& message) {
            std::stringstream ss;
            // Timestamp (simple)
            // ss << "[" << __TIME__ << "] "; 
            
            switch(level) {
                case LogLevel::DEBUG: ss << "[DEBUG] "; break;
                case LogLevel::INFO:  ss << "[INFO]  "; break;
                case LogLevel::WARN:  ss << "[WARN]  "; break;
                case LogLevel::ERR:   ss << "[ERROR] "; break;
            }
            
            ss << message;
            write(ss.str());
        }

        std::vector<std::string> getHistory() {
            std::lock_guard<std::mutex> lock(mutex_);
            return std::vector<std::string>(history_.begin(), history_.end());
        }

    private:
        Logger() = default;
        ~Logger() {
            if (fileStream_.is_open()) fileStream_.close();
        }
        
        std::ofstream fileStream_;
        std::mutex mutex_;
        std::deque<std::string> history_;

        void write(const std::string& message) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Console
            std::cout << message << std::endl;
            
            // File
            if (fileStream_.is_open()) {
                fileStream_ << message << std::endl;
                fileStream_.flush();
            }

            // History
            if (history_.size() > 50) history_.pop_front();
            history_.push_back(message);
        }
    };
}

// Helper macros
#define LSAA_LOG_INFO(msg) lsaa::Logger::instance().log(lsaa::LogLevel::INFO, msg)
#define LSAA_LOG_WARN(msg) lsaa::Logger::instance().log(lsaa::LogLevel::WARN, msg)
#define LSAA_LOG_ERROR(msg) lsaa::Logger::instance().log(lsaa::LogLevel::ERR, msg)
#define LSAA_LOG_DEBUG(msg) lsaa::Logger::instance().log(lsaa::LogLevel::DEBUG, msg)
