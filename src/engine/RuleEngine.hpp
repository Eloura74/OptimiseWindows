#pragma once
#include <vector>
#include <memory>
#include "Rule.hpp"

namespace lsaa {

    class RuleEngine {
    public:
        void addRule(Rule rule) {
            rules_.push_back(std::move(rule));
        }

        // Évalue toutes les règles par rapport aux métriques globales fusionnées
        void evaluate(const MetricsMap& metrics) {
            for (auto& rule : rules_) {
                rule.checkAndExecute(metrics);
            }
        }

    private:
        std::vector<Rule> rules_;
    };

}
