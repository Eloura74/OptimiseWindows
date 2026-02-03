#pragma once
#include <string>
#include <variant>
#include <functional>
#include <utility>
#include <memory>
#include <cmath>
#include "../core/IMonitor.hpp"
#include "../core/Logger.hpp"

namespace lsaa {

    // --- CONDITIONS ---

    class ICondition {
    public:
        virtual ~ICondition() = default;
        virtual bool evaluate(const MetricsMap& metrics) const = 0;
    };

    // Generic Metric Condition (e.g. "cpu_usage_percent" > 90.0)
    class ConditionGeneric : public ICondition {
    public:
        enum class Operator { GREATER, GREATER_EQUAL, LESS, LESS_EQUAL, EQUAL, NOT_EQUAL };

        ConditionGeneric(std::string metric, Operator op, double threshold)
            : metric_(std::move(metric)), op_(op), threshold_(threshold) {}

        bool evaluate(const MetricsMap& metrics) const override {
            auto it = metrics.find(metric_);
            if (it == metrics.end()) return false;

            double val = 0.0;
            const auto& v = it->second;
            
            if (std::holds_alternative<double>(v)) val = std::get<double>(v);
            else if (std::holds_alternative<long long>(v)) val = static_cast<double>(std::get<long long>(v));
            else return false;

            switch(op_) {
                case Operator::GREATER:       return val > threshold_;
                case Operator::GREATER_EQUAL: return val >= threshold_;
                case Operator::LESS:          return val < threshold_;
                case Operator::LESS_EQUAL:    return val <= threshold_;
                case Operator::EQUAL:         return std::abs(val - threshold_) < 0.001;
                case Operator::NOT_EQUAL:     return std::abs(val - threshold_) > 0.001;
            }
            return false;
        }
    private:
        std::string metric_;
        Operator op_;
        double threshold_;
    };

    // Specific CPU Condition Helper
    class ConditionCPU : public ConditionGeneric {
    public:
        ConditionCPU(double threshold) 
            : ConditionGeneric("cpu_usage_percent", Operator::GREATER, threshold) {}
    };

    // Specific RAM Condition Helper
    class ConditionRAM : public ConditionGeneric {
    public:
        ConditionRAM(double threshold) 
            : ConditionGeneric("ram_load_percent", Operator::GREATER, threshold) {}
    };


    // --- ACTIONS ---

    class IAction {
    public:
        virtual ~IAction() = default;
        virtual void execute() = 0;
        virtual std::string getName() const = 0;
    };

    class ActionLog : public IAction {
    public:
        enum class Level { INFO, WARN, ERR };
        ActionLog(Level level, std::string msg) : level_(level), msg_(std::move(msg)) {}
        void execute() override {
            if(level_ == Level::WARN) LSAA_LOG_WARN(msg_);
            else if(level_ == Level::ERR) LSAA_LOG_ERROR(msg_);
            else LSAA_LOG_INFO(msg_);
        }
        std::string getName() const override { return "LogAction"; }
    private:
        Level level_;
        std::string msg_;
    };


    // --- RULE ---

    class Rule {
    public:
        Rule(std::string name) : name_(std::move(name)) {}
        
        // Legacy Constructor support (optional but helpful)
        Rule(std::string name, std::unique_ptr<ICondition> cond, std::unique_ptr<IAction> action)
            : name_(std::move(name)), condition_(std::move(cond)), action_(std::move(action)) {}

        void setCondition(std::unique_ptr<ICondition> cond) { condition_ = std::move(cond); }
        void setAction(std::unique_ptr<IAction> action) { action_ = std::move(action); }

        void checkAndExecute(const MetricsMap& metrics) {
            if (!condition_ || !action_) return;

            bool currentStatus = condition_->evaluate(metrics);

            if (currentStatus) {
                if (!lastStatus_) {
                    LSAA_LOG_INFO("Rule Triggered: " + name_);
                    action_->execute();
                }
            } else {
                if (lastStatus_) {
                    LSAA_LOG_INFO("Rule Cleared: " + name_);
                }
            }
            lastStatus_ = currentStatus;
        }

    private:
        std::string name_;
        std::unique_ptr<ICondition> condition_;
        std::unique_ptr<IAction> action_;
        bool lastStatus_ = false;
    };

}
