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

    lsaa::Engine engine;
    g_engine = &engine;

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

    // 3. Configure Rules
    // ... (Keep existing rules or move to config loader)
    engine.getRuleEngine().addRule(
        lsaa::Rule("HighRAM",
            lsaa::Condition("ram_load_percent", lsaa::Operator::GREATER, 80.0),
            std::make_unique<lsaa::ActionLog>(lsaa::LogLevel::WARN, "ALERTE: Utilisation RAM critique (>80%) !")
        )
    );

    // 4. Init GUI
    auto& gui = lsaa::GuiManager::instance();
    if (!gui.init()) {
        LSAA_LOG_ERROR("Failed to init GUI. Exiting.");
        return 1;
    }

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

        auto topProcs = pmPtr->getTopProcesses();
        auto logs = lsaa::Logger::instance().getHistory();

        gui.drawDashboard(cpu, ramUsed, ramTotal, topProcs, logs, 
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
