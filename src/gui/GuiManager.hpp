#pragma once
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GL/gl.h>
#include <vector>
#include <string>
#include <functional>
#include <filesystem>
#include <map>
#include <algorithm>
#include <memory> 

#include "../core/Logger.hpp"
#include "../core/ConfigManager.hpp"
#include "../core/Lang.hpp"
#include "../modules/Cleaner.hpp"
#include "../modules/StartupManager.hpp"
#include "../modules/ServiceManager.hpp"
#include "../monitors/ProcessMonitor.hpp" // Required for ProcessInfo

namespace lsaa {

    class GuiManager {
        // --- SINGLETON ---
    public:
        static GuiManager& instance() {
            static GuiManager instance;
            return instance;
        }

        bool init() {
            if (!glfwInit()) return false;
            const char* glsl_version = "#version 130";
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
            
            // Fixed aspect ratio for professional feel
            window_ = glfwCreateWindow(1280, 800, "LSAA - Professional Agent", NULL, NULL);
            if (window_ == NULL) return false;
            
            glfwMakeContextCurrent(window_);
            glfwSwapInterval(1); 

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            
            // Load Fonts
            loadFonts();
            applyElegantTheme(); 

            ImGui_ImplGlfw_InitForOpenGL(window_, true);
            ImGui_ImplOpenGL3_Init(glsl_version);
            
            cleaner_ = std::make_unique<Cleaner>();
            return true;
        }

        bool shouldClose() { return glfwWindowShouldClose(window_); }
        
        void beginFrame() { 
            glfwPollEvents(); 
            ImGui_ImplOpenGL3_NewFrame(); 
            ImGui_ImplGlfw_NewFrame(); 
            ImGui::NewFrame(); 
        }

        void endFrame() { 
            ImGui::Render(); 
            int w, h; 
            glfwGetFramebufferSize(window_, &w, &h); 
            glViewport(0, 0, w, h); 
            // Theme Background Color (Matches WindowBg)
            glClearColor(0.09f, 0.09f, 0.10f, 1.0f); 
            glClear(GL_COLOR_BUFFER_BIT); 
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); 
            glfwSwapBuffers(window_); 
        }

        void cleanup() { 
            ImGui_ImplOpenGL3_Shutdown(); 
            ImGui_ImplGlfw_Shutdown(); 
            ImGui::DestroyContext(); 
            glfwDestroyWindow(window_); 
            glfwTerminate(); 
        }

        // --- MAIN RENDER LOOP ---
        void drawUI(double cpu, long long ramUsed, long long ramTotal, 
                          const std::vector<ProcessInfo>& topProcesses,
                          const std::vector<std::string>& logs, // Warning unused suppressed
                          std::function<void(DWORD)> onKillProcess,
                          std::function<void()> onRunScript,
                          std::function<void()> onSendNotification,
                          std::function<void()> onReloadEngine) 
        {
             updateHistory(cpu, ramUsed, ramTotal);

             // Fullscreen Window
             const ImGuiViewport* viewport = ImGui::GetMainViewport();
             ImGui::SetNextWindowPos(viewport->WorkPos);
             ImGui::SetNextWindowSize(viewport->WorkSize);
             ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
             ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
             ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); 
             
             ImGui::Begin("MainDock", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);
             ImGui::PopStyleVar(3);

             // --- LAYOUT: SIDEBAR (Left) + CONTENT (Right) ---
             
             // Sidebar Area
             float sidebarWidth = 240.0f;
             ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
             
             // Logo / Brand
             ImGui::Dummy(ImVec2(0, 30));
             ImGui::Indent(20);
             if (ImGui::GetIO().Fonts->Fonts.Size > 1) ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
             ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.9f), "L S A A");
             if (ImGui::GetIO().Fonts->Fonts.Size > 1) ImGui::PopFont();
             ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "System Intelligence");
             ImGui::Unindent(20);
             
             ImGui::Dummy(ImVec2(0, 50));

             // Navigation Buttons
             renderNavItem(Lang::instance().get("DASHBOARD"), 0);
             renderNavItem(Lang::instance().get("SERVICES"), 1);
             renderNavItem(Lang::instance().get("OPTIMIZER"), 2);
             renderNavItem(Lang::instance().get("RULES"), 3);
             renderNavItem(Lang::instance().get("STARTUP"), 4);

             // Bottom Sidebar info
             ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 100);
             ImGui::Indent(20);
             ImGui::Separator();
             ImGui::Dummy(ImVec2(0, 10));
             
             // Lang Toggle
             if (ImGui::Button(Lang::instance().getLanguage() == Language::EN ? "EN / FR" : "FR / EN", ImVec2(80, 25))) {
                 Lang::instance().toggle();
             }
             ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.2f), "v1.0.0-beta");
             ImGui::Unindent(20);

             ImGui::EndChild(); // End Sidebar

             ImGui::SameLine(); // Go to right side

             // Content Area
             ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); 
             ImGui::BeginChild("Content", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
             ImGui::PopStyleColor();

             ImGui::Dummy(ImVec2(0, 10));
             
             // Switch content based on active tab
             switch (activeTab_) {
                 case 0:
                     renderDashboard(cpu, ramUsed, ramTotal, topProcesses, logs, onKillProcess, onRunScript, onSendNotification);
                     break;
                 case 1:
                     renderServiceManager();
                     break;
                 case 2:
                     renderOptimizer();
                     break;
                 case 3:
                     renderRulesEditor(onReloadEngine);
                     break;
                 case 4:
                     renderStartupManager();
                     break;
             }

             ImGui::EndChild(); // End Content

             ImGui::End(); // End Main Window
        }

    private:
        int activeTab_ = 0;
        std::unique_ptr<Cleaner> cleaner_;
        Cleaner::ScanResult lastScan_ = {0, 0};
        
        // History for Graphs
        std::vector<float> historyCpu_;
        std::vector<float> historyRam_;

        // --- COMPONENTS & THEME ---

        void renderNavItem(const char* label, int index) {
            bool isActive = (activeTab_ == index);
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.1f, 0.5f)); // Left align text
            
            ImVec4 btnColor = isActive ? ImVec4(0.18f, 0.18f, 0.20f, 1.0f) : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            ImVec4 txtColor = isActive ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
            ImGui::PushStyleColor(ImGuiCol_Text, txtColor);
            
            // Full width button
            if (ImGui::Button(label, ImVec2(-20, 45))) { // -20 for right padding
                activeTab_ = index;
            }
            
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            ImGui::Dummy(ImVec2(0, 5));
        }

        void BeginCard(const char* name, float height_factor = 0.0f) {
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
            // Card Background: Slightly lighter than bg
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.16f, 1.0f)); 
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.03f)); 
            
            float h = height_factor > 0 ? ImGui::GetContentRegionAvail().y * height_factor : 0;
            if (height_factor == 0) h = 0; 
            
            ImGui::BeginChild(name, ImVec2(0, h), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
            ImGui::PopStyleColor(2); 
            ImGui::PopStyleVar(3);   
        }

        void EndCard() {
             ImGui::EndChild();
        }

        void renderDashboard(double cpu, long long ramUsed, long long ramTotal, 
                          const std::vector<ProcessInfo>& topProcesses,
                          const std::vector<std::string>& logs,
                          std::function<void(DWORD)> onKillProcess,
                          std::function<void()> onRunScript,
                          std::function<void()> onSendNotification) 
        {
             // Modern Header within Dashboard
             ImGui::TextColored(ImVec4(1,1,1,0.5f), Lang::instance().get("DASHBOARD"));
             ImGui::SetWindowFontScale(1.5f);
             ImGui::Text("System Overview");
             ImGui::SetWindowFontScale(1.0f);
             ImGui::Dummy(ImVec2(0, 20));

             float width = ImGui::GetContentRegionAvail().x;
             float col1W = width * 0.5f - 10;
             
             // Top Row: 2 Cards (CPU / RAM)
             ImGui::BeginChild("Row1", ImVec2(0, 200), false);
             
             // CPU Card
             ImGui::BeginChild("CPU_Card_Container", ImVec2(col1W, 0), false);
             BeginCard("CPUMonCard");
             {
                 ImGui::TextColored(ImVec4(1,1,1,0.7f), Lang::instance().get("PROCESSOR_LOAD"));
                 ImGui::SetWindowFontScale(2.0f);
                 ImGui::Text("%.1f%%", cpu);
                 ImGui::SetWindowFontScale(1.0f);
                 
                 ImGui::Dummy(ImVec2(0, 15));
                 ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
                 ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
                 ImGui::PlotLines("##CPU", historyCpu_.data(), (int)historyCpu_.size(), 0, NULL, 0.0f, 100.0f, ImVec2(ImGui::GetContentRegionAvail().x, 60));
                 ImGui::PopStyleColor(2);
             }
             EndCard();
             ImGui::EndChild();

             ImGui::SameLine();
             
             // RAM Card
             ImGui::BeginChild("RAM_Card_Container", ImVec2(col1W, 0), false);
             BeginCard("RAMMonCard");
             {
                 ImGui::TextColored(ImVec4(1,1,1,0.7f), Lang::instance().get("MEMORY_USAGE"));
                 float ramPercent = (ramTotal > 0) ? (float)((double)ramUsed / ramTotal * 100.0) : 0;
                 ImGui::SetWindowFontScale(2.0f);
                 ImGui::Text("%.1f%%", ramPercent);
                 ImGui::SetWindowFontScale(1.0f);
                 
                 ImGui::SameLine();
                 ImGui::TextColored(ImVec4(1,1,1,0.4f), "(%.1f / %.1f GB)", ramUsed / 1024.0 / 1024.0 / 1024.0, ramTotal / 1024.0 / 1024.0 / 1024.0);
                 
                 ImGui::Dummy(ImVec2(0, 15));
                 ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
                 ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
                 ImGui::ProgressBar(ramPercent / 100.0f, ImVec2(-1, 8));
                 ImGui::PopStyleColor(2);
             }
             EndCard();
             ImGui::EndChild();

             ImGui::EndChild(); // Row 1

             ImGui::Dummy(ImVec2(0, 20));

             // Row 2: Processes (Big) + Actions (Small side)
             float actionWidth = 280.0f;
             if (width < 800) actionWidth = 200; // Responsiveish
             float processWidth = width - actionWidth - 20;

             // Processes
             ImGui::BeginChild("Proc_Col", ImVec2(processWidth, 0), false);
             BeginCard("ProcPanel", 0.0f); 
             {
                 ImGui::TextColored(ImVec4(1,1,1,0.8f), Lang::instance().get("TOP_PROCESSES"));
                 ImGui::Dummy(ImVec2(0, 15));
                 
                 // Elegant Table
                 if (ImGui::BeginTable("table_processes", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoBordersInBody)) {
                        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                        ImGui::TableSetupColumn(Lang::instance().get("NAME"), ImGuiTableColumnFlags_WidthStretch); 
                        ImGui::TableSetupColumn("MEM", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f); // Actions
                        ImGui::TableHeadersRow();

                        for (const auto& p : topProcesses) {
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 40.0f); // Taller rows
                            ImGui::TableNextColumn(); ImGui::TextDisabled("%d", p.pid);
                            ImGui::TableNextColumn(); ImGui::Text("%s", p.name.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("%.0f MB", p.memoryBytes / 1024.0 / 1024.0);
                            ImGui::TableNextColumn();
                            
                            ImGui::PushID(p.pid);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 0.3f, 0.1f));
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                            if (ImGui::Button("KILL", ImVec2(60, 26))) if (onKillProcess) onKillProcess(p.pid);
                            ImGui::PopStyleColor(2);
                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                 }
             }
             EndCard();
             ImGui::EndChild();

             ImGui::SameLine();

             // Actions
             ImGui::BeginChild("Act_Col", ImVec2(actionWidth, 0), false);
             BeginCard("ActionPanel", 0.0f);
             {
                 ImGui::TextColored(ImVec4(1, 1, 1, 0.8f), Lang::instance().get("QUICK_ACTIONS"));
                 ImGui::Dummy(ImVec2(0, 15));
                 
                 // Big Buttons
                 ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
                 ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
                 
                 if (ImGui::Button(Lang::instance().get("RUN_CLEANER"), ImVec2(-1, 50))) if (onRunScript) onRunScript();
                 ImGui::Dummy(ImVec2(0, 10));
                 if (ImGui::Button(Lang::instance().get("TEST_NOTIF"), ImVec2(-1, 50))) if (onSendNotification) onSendNotification();
                 
                 ImGui::PopStyleColor(2);
             }
             EndCard();
             ImGui::EndChild();
        }

        void renderServiceManager() {
             ImGui::TextColored(ImVec4(1,1,1,0.5f), "SYSTEM");
             ImGui::SetWindowFontScale(1.5f);
             ImGui::Text(Lang::instance().get("SERVICES"));
             ImGui::SetWindowFontScale(1.0f);
             ImGui::Dummy(ImVec2(0, 20));

             BeginCard("ServicesList", 0.0f);
             static std::vector<ServiceInfo> services = ServiceManager::getServices();
             static std::string filter = "";

             ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
             if (ImGui::Button("Refresh", ImVec2(120, 35))) services = ServiceManager::getServices();
             ImGui::PopStyleColor();

             ImGui::SameLine();
             static char searchBuf[128] = "";
             ImGui::SetNextItemWidth(300);
             if (ImGui::InputTextWithHint("##search", "Search...", searchBuf, 128)) filter = std::string(searchBuf);

             ImGui::Dummy(ImVec2(0, 20));

             if (ImGui::BeginTable("ServicesTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoBordersInBody)) {
                // ... (Same columns as before)
                ImGui::TableSetupColumn(Lang::instance().get("NAME"), ImGuiTableColumnFlags_WidthFixed, 200.0f);
                ImGui::TableSetupColumn(Lang::instance().get("DISPLAY_NAME"), ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn(Lang::instance().get("PID"), ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn(Lang::instance().get("STATUS"), ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();

                for (auto& svc : services) {
                     if (!filter.empty()) { 
                        std::string lowerName = svc.name;
                        std::string lowerDisplay = svc.displayName;
                        std::string lowerFilter = filter;
                        // Manual tolower loop with cast to fix warnings
                        for(auto& c : lowerName) c = (char)tolower(c);
                        for(auto& c : lowerDisplay) c = (char)tolower(c);
                        for(auto& c : lowerFilter) c = (char)tolower(c);

                        if (lowerName.find(lowerFilter) == std::string::npos && lowerDisplay.find(lowerFilter) == std::string::npos) continue;
                     }

                    ImGui::PushID(svc.name.c_str());
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, 40.0f);
                    
                    ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(1,1,1,0.9f), "%s", svc.name.c_str());
                    ImGui::TableNextColumn(); ImGui::TextDisabled("%s", svc.displayName.c_str());
                    ImGui::TableNextColumn(); ImGui::TextDisabled("%d", svc.pid);
                    
                    ImGui::TableNextColumn(); 
                    // Draw Status Circle
                    if (svc.stateCode == SERVICE_RUNNING || svc.stateCode == SERVICE_STOPPED) {
                         ImVec2 p = ImGui::GetCursorScreenPos();
                         ImU32 col = (svc.stateCode == SERVICE_RUNNING) ? IM_COL32(100, 220, 120, 255) : IM_COL32(100, 100, 100, 255);
                         ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 8, p.y + 18), 4.0f, col);
                         ImGui::Dummy(ImVec2(15, 0)); ImGui::SameLine();
                         if (svc.stateCode == SERVICE_RUNNING) ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "Running");
                         else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Stopped");
                    }
                    else ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.0f, 1.0f), "%s", svc.status.c_str());

                    ImGui::TableNextColumn();
                    // Clean Text Buttons for actions
                    if (svc.canStop) {
                        if (ImGui::SmallButton("Stop")) { ServiceManager::stopService(svc.name); svc.status = "Stopping"; svc.canStop=false; }
                    } else if (svc.stateCode == SERVICE_STOPPED) {
                        if (ImGui::SmallButton("Start")) { ServiceManager::startService(svc.name); svc.status = "Starting"; }
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
             }
             EndCard();
        }

        void renderOptimizer() {
            static CleanParams params; 
            ImGui::TextColored(ImVec4(1,1,1,0.5f), "CLEANER");
            ImGui::SetWindowFontScale(1.5f);
            ImGui::Text("System Optimization");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Dummy(ImVec2(0, 20));

            BeginCard("CleanCard", 0.0f);
            ImGui::TextColored(ImVec4(1,1,1,0.8f), "Select Targets");
            ImGui::Dummy(ImVec2(0, 15));

            ImGui::Checkbox(Lang::instance().get("SYS_TEMP"), &params.sysTemp);
            ImGui::Checkbox(Lang::instance().get("CHROME_CACHE"), &params.chrome);
            ImGui::Checkbox(Lang::instance().get("EDGE_CACHE"), &params.edge);
            ImGui::Checkbox(Lang::instance().get("FIREFOX_CACHE"), &params.firefox);
            ImGui::Checkbox(Lang::instance().get("DNS_CACHE"), &params.dns);

            ImGui::Dummy(ImVec2(0, 30));

            // Big CTA Button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
            if (ImGui::Button(Lang::instance().get("SCAN_NOW"), ImVec2(200, 50))) lastScan_ = cleaner_->scan(params);
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0, 20));
            if (lastScan_.fileCount > 0) {
                 ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.5f, 1.0f), "%zu files found (%.1f MB)", lastScan_.fileCount, lastScan_.totalSize / 1024.0 / 1024.0);
                 ImGui::Dummy(ImVec2(0, 10));
                 
                 ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 0.8f));
                 ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                 if (ImGui::Button(Lang::instance().get("CLEAN_ALL"), ImVec2(200, 50))) { cleaner_->clean(params); lastScan_ = cleaner_->scan(params); }
                 ImGui::PopStyleColor(2);
            }
            EndCard();
        }

        void renderStartupManager() {
            ImGui::TextColored(ImVec4(1,1,1,0.5f), "SYSTEM");
            ImGui::SetWindowFontScale(1.5f);
            ImGui::Text(Lang::instance().get("STARTUP_MANAGER"));
            ImGui::SetWindowFontScale(1.0f);
             ImGui::Dummy(ImVec2(0, 20));
             BeginCard("StartupList");
             // ... Simple Table ...
             static std::vector<StartupItem> items = StartupManager::getStartupItems();
             if (ImGui::Button("Refresh")) items = StartupManager::getStartupItems();
             ImGui::Dummy(ImVec2(0, 10));
             if (ImGui::BeginTable("StartupTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn(Lang::instance().get("NAME"));
                ImGui::TableSetupColumn(Lang::instance().get("PATH"));
                ImGui::TableSetupColumn("");
                ImGui::TableHeadersRow();
                 for (auto& item : items) {
                     ImGui::TableNextRow(ImGuiTableRowFlags_None, 40.0f);
                     ImGui::TableNextColumn(); ImGui::Text("%s", item.name.c_str());
                     ImGui::TableNextColumn(); ImGui::TextDisabled("%s", item.path.c_str());
                     ImGui::TableNextColumn();
                     if (ImGui::Button("Delete")) { StartupManager::removeItem(item.name); items = StartupManager::getStartupItems(); }
                 }
                 ImGui::EndTable();
             }
             EndCard();
        }

        void renderRulesEditor(std::function<void()> onReloadEngine) {
             ImGui::TextColored(ImVec4(1,1,1,0.5f), "AUTOMATION");
             ImGui::SetWindowFontScale(1.5f);
             ImGui::Text("Rules Engine");
             ImGui::SetWindowFontScale(1.0f);
             ImGui::Dummy(ImVec2(0, 20));
             
             BeginCard("RulesCards");
             // Minimal placeholder for rules
             ImGui::TextDisabled("Configuration Editor");
             ImGui::Dummy(ImVec2(0, 10));
             if(ImGui::Button(Lang::instance().get("SAVE_CONFIG"), ImVec2(150, 40))) { ConfigManager::instance().save(); if(onReloadEngine) onReloadEngine(); }
             EndCard();
        }

        void updateHistory(double cpu, long long ramUsed, long long ramTotal) {
             if (historyCpu_.size() > 120) historyCpu_.erase(historyCpu_.begin());
             historyCpu_.push_back((float)cpu);
             float ramPercent = 0.0f;
             if (ramTotal > 0) ramPercent = (float)((double)ramUsed / (double)ramTotal * 100.0);
             if (historyRam_.size() > 120) historyRam_.erase(historyRam_.begin());
             historyRam_.push_back(ramPercent);
        }

        void applyElegantTheme() {
            ImGuiStyle& style = ImGui::GetStyle();
            
            style.WindowRounding = 0.0f;
            style.ChildRounding = 12.0f;
            style.FrameRounding = 6.0f;
            style.PopupRounding = 6.0f;
            style.ScrollbarRounding = 12.0f;
            style.GrabRounding = 6.0f;
            
            style.WindowPadding = ImVec2(20, 20);
            style.FramePadding = ImVec2(10, 8);
            style.ItemSpacing = ImVec2(12, 12);
            style.IndentSpacing = 20.0f;
            style.ScrollbarSize = 10.0f;

            // Colors: ELEGANT DARK (Monochrome + Slate)
            ImVec4 bg             = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
            ImVec4 sidebar        = ImVec4(0.09f, 0.09f, 0.10f, 1.00f); // Same as BG mostly
            ImVec4 card           = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
            ImVec4 text           = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
            ImVec4 textSec        = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            ImVec4 border         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            ImVec4 accent         = ImVec4(1.00f, 1.00f, 1.00f, 0.10f); // Subtle White highlight
            
            style.Colors[ImGuiCol_Text] = text;
            style.Colors[ImGuiCol_TextDisabled] = textSec;
            style.Colors[ImGuiCol_WindowBg] = bg;
            style.Colors[ImGuiCol_ChildBg] = card;
            style.Colors[ImGuiCol_Border] = border;
            
            style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 0.05f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.12f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.18f);

            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.2f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.05f);
            
            style.Colors[ImGuiCol_Header] = accent;
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
            
            style.Colors[ImGuiCol_ScrollbarBg] = bg;
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.3f);
        }

        void loadFonts() {
            ImGuiIO& io = ImGui::GetIO();
             if (std::filesystem::exists("C:\\Windows\\Fonts\\segoeui.ttf")) {
                io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f); // Normal
                io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 36.0f); // Large Title
             } else {
                 io.Fonts->AddFontDefault();
             }
        }
        
        GuiManager() {
            historyCpu_.reserve(120);
            historyRam_.reserve(120);
        }
        ~GuiManager() = default;

        GLFWwindow* window_ = nullptr;
    };
}
