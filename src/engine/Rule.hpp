#pragma once
#include <string>
#include <variant>
#include <functional>
#include <utility>
#include "../core/IMonitor.hpp"
#include "../core/Logger.hpp"

namespace lsaa {

    // Opérateurs de comparaison supportés
    enum class Operator {
        GREATER,
        GREATER_EQUAL,
        LESS,
        LESS_EQUAL,
        EQUAL,
        NOT_EQUAL
    };

    // Une condition porte sur une métrique spécifique
    class Condition {
    public:
        Condition(std::string metric, Operator op, double threshold)
            : metric_(std::move(metric)), op_(op), threshold_(threshold) {}

        bool evaluate(const MetricsMap& metrics) const {
            auto it = metrics.find(metric_);
            if (it == metrics.end()) return false;

            // On essaie de convertir la valeur en double pour la comparaison
            double val = 0.0;
            const auto& v = it->second;
            
            if (std::holds_alternative<double>(v)) {
                val = std::get<double>(v);
            } else if (std::holds_alternative<long long>(v)) {
                val = static_cast<double>(std::get<long long>(v));
            } else {
                // String comparison not supported for numeric operators in this MVP
                return false;
            }

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

    // Interface générique pour une action
    class IAction {
    public:
        virtual ~IAction() = default;
        virtual void execute() = 0;
        virtual std::string getName() const = 0;
    };

    // Action simple : Log
    class ActionLog : public IAction {
    public:
        ActionLog(LogLevel level, std::string msg) 
            : level_(level), msg_(std::move(msg)) {}

        void execute() override {
            // On utilise le logger global
            switch(level_) {
                case LogLevel::INFO:  LSAA_LOG_INFO(msg_); break;
                case LogLevel::WARN:  LSAA_LOG_WARN(msg_); break;
                case LogLevel::ERR:   LSAA_LOG_ERROR(msg_); break;
                default: LSAA_LOG_DEBUG(msg_); break;
            }
        }
        
        std::string getName() const override { return "LogAction"; }

    private:
        LogLevel level_;
        std::string msg_;
    };

    // Une Règle = Nom + Condition + Action
    class Rule {
    public:
        Rule(std::string name, Condition cond, std::unique_ptr<IAction> action)
            : name_(std::move(name)), condition_(std::move(cond)), action_(std::move(action)) {}

        void checkAndExecute(const MetricsMap& metrics) {
            bool currentStatus = condition_.evaluate(metrics);

            // Logique simple pour l'instant : si condition vraie, on exécute
            // TODO: Ajouter state (cooldown, duration) pour éviter le flapping
            if (currentStatus) {
                if (!lastStatus_) {
                    // Rising edge (vient de devenir vrai)
                    LSAA_LOG_INFO("Rule Triggered: " + name_);
                    action_->execute();
                }
                // Si on veut répéter l'action tant que c'est vrai, on le fait ici
                // Pour l'instant on fait "Edge Triggered" (seulement au changement)
            } else {
                if (lastStatus_) {
                    // Falling edge (vient de devenir faux)
                    LSAA_LOG_INFO("Rule Cleared: " + name_);
                }
            }

            lastStatus_ = currentStatus;
        }

    private:
        std::string name_;
        Condition condition_;
        std::unique_ptr<IAction> action_;
        bool lastStatus_ = false;
    };

}
