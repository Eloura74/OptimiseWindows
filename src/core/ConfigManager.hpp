#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include "../core/Logger.hpp"

using json = nlohmann::json;

namespace lsaa {

    struct RuleConfig {
        std::string name;
        std::string metric;     // e.g., "cpu_usage_percent"
        std::string oper;       // e.g., ">"
        double threshold;
        bool enabled;
        std::string actionType; // "LOG", "NOTIFY", "KILL"
        std::string actionParam; // message or target
    };

    // JSON Serialization for RuleConfig
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RuleConfig, name, metric, oper, threshold, enabled, actionType, actionParam)

    class ConfigManager {
    public:
        static ConfigManager& instance() {
            static ConfigManager instance;
            return instance;
        }

        void load(const std::string& filename = "rules.json") {
            filename_ = filename;
            std::ifstream file(filename);
            if (file.is_open()) {
                try {
                    json j;
                    file >> j;
                    rules_ = j.get<std::vector<RuleConfig>>();
                    LSAA_LOG_INFO("Configuration loaded: " + std::to_string(rules_.size()) + " rules.");
                } catch (const std::exception& e) {
                    LSAA_LOG_ERROR("Failed to parse config: " + std::string(e.what()));
                    createDefaultConfig();
                }
            } else {
                LSAA_LOG_WARN("Config file not found. Creating default.");
                createDefaultConfig();
            }
        }

        void save() {
            std::ofstream file(filename_);
            if (file.is_open()) {
                json j = rules_;
                file << j.dump(4);
                LSAA_LOG_INFO("Configuration saved.");
            }
        }

        std::vector<RuleConfig>& getRules() { return rules_; }

    private:
        ConfigManager() = default;
        std::string filename_;
        std::vector<RuleConfig> rules_;

        void createDefaultConfig() {
            rules_ = {
                {"HighCPU", "cpu_usage_percent", ">", 90.0, true, "NOTIFY", "Alerte CPU Critique !"},
                {"HighRAM", "ram_load_percent", ">", 85.0, true, "LOG", "Utilisation RAM elevee"},
                {"TooManyProcs", "process_count", ">", 250.0, true, "LOG", "Nombre de processus anormal"}
            };
            save(); // Save defaults
        }
    };
}
