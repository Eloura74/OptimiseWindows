#include "core/Logger.hpp"
#include "core/Engine.hpp"
#include "monitors/SystemMonitor.hpp"
#include "monitors/ProcessMonitor.hpp"
#include "actions/ActionScript.hpp"
#include "actions/ActionProcess.hpp"
#include "actions/ActionNotification.hpp"
#include <csignal>

#include "gui/GuiManager.hpp"
#include <thread>

// Global pointer for signal handler
lsaa::Engine* g_engine = nullptr;

void signalHandler(int signum) {
    if (g_engine) {
        LSAA_LOG_INFO("Interrupt signal (" + std::to_string(signum) + ") received. Stopping...");
        g_engine->stop();
    }
}

int main() {
    // 1. Init System
    lsaa::Logger::instance().init("lsaa.log");
    LSAA_LOG_INFO("------------------------------------------");
    LSAA_LOG_INFO("   LSAA Core System Starting (Phase 4)    ");
    LSAA_LOG_INFO("------------------------------------------");
    // 3. Setup Engine
    lsaa::Engine engine;
    g_engine = &engine; // Assign global engine pointer
    
    // Handle Ctrl+C
    signal(SIGINT, signalHandler);

    // 2. Add Modules
    engine.addMonitor(std::make_unique<lsaa::CpuMonitor>());
    engine.addMonitor(std::make_unique<lsaa::RamMonitor>());
    
    // Keep raw pointer to ProcessMonitor for GUI access
    auto pm = std::make_unique<lsaa::ProcessMonitor>();
    auto* pmPtr = pm.get();
    engine.addMonitor(std::move(pm));
    // engine.addMonitor(std::make_unique<lsaa::GpuMonitor>()); // Future

    // Init Config
    lsaa::ConfigManager::instance().load();
    auto& rules = lsaa::ConfigManager::instance().getRules();

    for (const auto& cfg : rules) {
        if (!cfg.enabled) continue;

        // Factory mini-logic
        std::unique_ptr<lsaa::ICondition> cond;
        if (cfg.metric == "cpu_usage_percent") cond = std::make_unique<lsaa::ConditionCPU>(cfg.threshold);
        else if (cfg.metric == "ram_load_percent") cond = std::make_unique<lsaa::ConditionRAM>(cfg.threshold);
        else continue; // Unsupported metric for now

        std::unique_ptr<lsaa::IAction> action;
        if (cfg.actionType == "NOTIFY") action = std::make_unique<lsaa::ActionNotification>("LSAA Alert", cfg.actionParam);
        else continue; // Unsupported action for now (Logging is default in engine usually?)
        
        // Note: The current Engine/Rule design might need adjustment to support dynamic generic rules perfectly, 
        // but let's try to adapt.
        // Actually, the previous hardcoded rules used specific classes.
        // Let's stick to the mapped logic.
        
        auto rule = std::make_unique<lsaa::Rule>(cfg.name);
        rule->setCondition(std::move(cond));
        rule->setAction(std::move(action));
        engine.addRule(std::move(rule));
    }

    // Default Fallback if no rules (safety)
    if (rules.empty()) {
        auto ruleCPU = std::make_unique<lsaa::Rule>("HighCPU_Legacy");
        ruleCPU->setCondition(std::make_unique<lsaa::ConditionCPU>(90.0));
        ruleCPU->setAction(std::make_unique<lsaa::ActionNotification>("Alerte CPU", "CPU > 90%"));
        engine.addRule(std::move(ruleCPU));
    }

    // 4. Init GUI
    auto& gui = lsaa::GuiManager::instance();
    if (!gui.init()) {
        LSAA_LOG_ERROR("Failed to init GUI. Exiting.");
        return 1;
    }

    // Init Config
    lsaa::ConfigManager::instance().load();

    // 5. Start Engine in background thread
    std::thread engineThread([&engine]() {
        engine.run(); 
    });

    // 6. Main GUI Loop
    while (!gui.shouldClose()) {
        gui.beginFrame();

        // Get Data from Engine
        auto metrics = engine.getLastMetrics();
        
        // Extract basic metrics for Dashboard (MVP)
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
            }
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
