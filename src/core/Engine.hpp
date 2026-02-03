#pragma once
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <sstream>
#include <iomanip>
#include "IMonitor.hpp"
#include "Logger.hpp"
#include "../engine/RuleEngine.hpp"

namespace lsaa {

    class Engine {
    public:
        Engine() : running_(false) {}

        void addMonitor(std::unique_ptr<IMonitor> monitor) {
            monitors_.push_back(std::move(monitor));
        }

        void addRule(std::unique_ptr<Rule> rule) {
            ruleEngine_.addRule(std::move(rule));
        }
        
        void clearRules() {
            ruleEngine_.clear();
        }

        RuleEngine& getRuleEngine() { return ruleEngine_; }

        void run() {
             // Legacy run blocking
             running_ = true;
             while(running_) {
                 auto start = std::chrono::steady_clock::now();
                 step();
                 std::this_thread::sleep_until(start + std::chrono::milliseconds(1000));
             }
        }

        void step() {
             // 1. Collect
             MetricsMap currentMetrics;
             for (auto& mon : monitors_) {
                 if (mon->collect()) {
                      auto m = mon->getMetrics();
                      currentMetrics.insert(m.begin(), m.end());
                      // logMetrics(mon.get()); // Trop verbeux si 60fps
                 }
             }
             std::lock_guard<std::mutex> lock(metricsMutex_);
             lastMetrics_ = currentMetrics;

             // 2. Rules
             ruleEngine_.evaluate(currentMetrics);
        }

        MetricsMap getLastMetrics() {
             std::lock_guard<std::mutex> lock(metricsMutex_);
             return lastMetrics_;
        }
        
        void stop() {
            running_ = false;
        }

    private:
        std::mutex metricsMutex_;
        MetricsMap lastMetrics_;

    private:
        void logMetrics(IMonitor* mon) {
            auto metrics = mon->getMetrics();
            std::stringstream ss;
            ss << "[" << mon->getName() << "] ";
            
            for (const auto& [key, value] : metrics) {
                ss << key << "=";
                if (std::holds_alternative<long long>(value)) 
                    ss << std::get<long long>(value);
                else if (std::holds_alternative<double>(value))
                    ss << std::fixed << std::setprecision(1) << std::get<double>(value);
                else
                    ss << std::get<std::string>(value);
                ss << " ";
            }
            LSAA_LOG_INFO(ss.str());
        }

        std::vector<std::unique_ptr<IMonitor>> monitors_;
        RuleEngine ruleEngine_;
        std::atomic<bool> running_;
    };
}
