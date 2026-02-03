#include <iostream>
#include <thread>
#include <chrono>
#include "core/Engine.hpp"
#include "monitors/ProcessMonitor.hpp"
#include "monitors/SystemMonitor.hpp"
#include "core/Logger.hpp"
#include "core/ConfigManager.hpp"
#include "engine/Rule.hpp"
#include "gui/GuiManager.hpp"

// Actions Lib
#include "actions/ActionProcess.hpp"
#include "actions/ActionScript.hpp"
#include "actions/ActionNotification.hpp"

int main() {
    lsaa::Logger::instance().log(lsaa::LogLevel::INFO, "LSAA Core System Starting (Phase 6)");

    // 1. Init Engine
    lsaa::Engine engine;

    // 2. Add Monitors
    // Process Monitor (Top Apps)
    auto pm = std::make_unique<lsaa::ProcessMonitor>();
    auto* pmPtr = pm.get(); // Keep raw ptr for GUI access
    engine.addMonitor(std::move(pm));

    // System Monitor (CPU/RAM Global)
    engine.addMonitor(std::make_unique<lsaa::SystemMonitor>());

    // 4. Init GUI
    auto& gui = lsaa::GuiManager::instance();
    if (!gui.init()) {
        LSAA_LOG_ERROR("Failed to init GUI. Exiting.");
        return 1;
    }

    // Helper to reload rules
    auto reloadRulesFn = [&engine]() {
        LSAA_LOG_INFO("Hot Reloading Rules...");
        engine.clearRules();
        auto& rules = lsaa::ConfigManager::instance().getRules();
        for (const auto& cfg : rules) {
            if (!cfg.enabled) continue;
            std::unique_ptr<lsaa::ICondition> cond;
            if (cfg.metric == "cpu_usage_percent") cond = std::make_unique<lsaa::ConditionCPU>(cfg.threshold);
            else if (cfg.metric == "ram_load_percent") cond = std::make_unique<lsaa::ConditionRAM>(cfg.threshold);
            else continue; 
            
            std::unique_ptr<lsaa::IAction> action;
            if (cfg.actionType == "NOTIFY") action = std::make_unique<lsaa::ActionNotification>("LSAA Alert", cfg.actionParam);
            else continue; 

            auto rule = std::make_unique<lsaa::Rule>(cfg.name);
            rule->setCondition(std::move(cond));
            rule->setAction(std::move(action));
            engine.addRule(std::move(rule));
        }
        LSAA_LOG_INFO("Rules Reloaded: " + std::to_string(rules.size()) + " attempted.");
    };

    // Initial Load
    lsaa::ConfigManager::instance().load();
    reloadRulesFn();

    // 5. Start Engine in background thread
    std::thread engineThread([&engine]() {
        engine.run(); 
    });

    // 6. Main GUI Loop
    while (!gui.shouldClose()) {
        gui.beginFrame();

        // Get Data from Engine
        auto metrics = engine.getLastMetrics();
        
        // Extract basic metrics for Dashboard
        double cpu = 0.0;
        long long ramUsed = 0;
        long long ramTotal = 1; // avoid div/0

        if (metrics.count("cpu_usage_percent")) cpu = std::get<double>(metrics.at("cpu_usage_percent"));
        if (metrics.count("ram_used_bytes")) ramUsed = std::get<long long>(metrics.at("ram_used_bytes"));
        if (metrics.count("ram_total_bytes")) ramTotal = std::get<long long>(metrics.at("ram_total_bytes"));

        // Get extended data
        auto topProcs = pmPtr->getTopProcesses();
        auto logs = lsaa::Logger::instance().getHistory();

        gui.drawUI(cpu, ramUsed, ramTotal, topProcs, logs, 
            // On Kill
            [](DWORD pid) {
                lsaa::ActionKillProcess action(pid);
                action.execute();
            },
            // On Script
            []() {
                lsaa::ActionScript action("cmd.exe /c echo Manual Action Triggered ! > build\\manual_trigger.txt");
                action.execute();
            },
            // On Notification
            []() {
                lsaa::ActionNotification action("LSAA Test", "Ceci est une notification de test !");
                action.execute();
            },
            // On Reload Engine (Hot Reload)
            reloadRulesFn
        );

        gui.endFrame();
        
        // Cap at ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Shutdown
    engine.stop();
    if (engineThread.joinable()) engineThread.join();
    
    gui.cleanup();
    LSAA_LOG_INFO("System stopped cleanly.");
    return 0;
}
