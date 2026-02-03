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
#include "../core/Logger.hpp"
#include "../core/ConfigManager.hpp"
#include "../core/Lang.hpp"
#include "../modules/Cleaner.hpp"
#include "../modules/StartupManager.hpp"
#include "../modules/ServiceManager.hpp"

namespace lsaa {

    struct ProcessInfo; 

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
            
            // Un peu plus grand par défaut
            window_ = glfwCreateWindow(1400, 900, "LSAA - System Automation Agent", NULL, NULL);
            if (window_ == NULL) return false;
            
            glfwMakeContextCurrent(window_);
            glfwSwapInterval(1); 

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            
            // Style & Fonts
            loadFonts();
            applyProTheme(); 

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
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f); // Pure Dark
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
                          const std::vector<std::string>& logs,
                          std::function<void(DWORD)> onKillProcess,
                          std::function<void()> onRunScript,
                          std::function<void()> onSendNotification,
                          std::function<void()> onReloadEngine) 
        {
             updateHistory(cpu, ramUsed, ramTotal);

             const ImGuiViewport* viewport = ImGui::GetMainViewport();
             ImGui::SetNextWindowPos(viewport->WorkPos);
             ImGui::SetNextWindowSize(viewport->WorkSize);
             ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
             ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
             ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24)); // More padding outer
             
             ImGui::Begin("MainDock", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);
             ImGui::PopStyleVar(3);

             // HEADER CARD
             renderHeader(cpu, historyRam_.empty() ? 0 : historyRam_.back());
             
             ImGui::Dummy(ImVec2(0, 20)); // Spacer

             // TABS WRAPPER
             ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
             ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
             ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.0f, 0.48f, 0.8f, 1.0f));
             
             if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {
                 
                 // --- DASHBOARD ---
                 if (ImGui::BeginTabItem(Lang::instance().get("DASHBOARD"))) {
                     ImGui::Dummy(ImVec2(0, 15));
                     drawInfoBanner(Lang::instance().get("DESC_DASHBOARD"));
                     renderDashboard(cpu, ramUsed, ramTotal, topProcesses, logs, onKillProcess, onRunScript, onSendNotification);
                     ImGui::EndTabItem();
                 }

                 // --- RULES ---
                 if (ImGui::BeginTabItem(Lang::instance().get("RULES"))) {
                     ImGui::Dummy(ImVec2(0, 15));
                     drawInfoBanner(Lang::instance().get("DESC_RULES"));
                     renderRulesEditor(onReloadEngine);
                     ImGui::EndTabItem();
                 }

                 // --- OPTIMIZER ---
                 if (ImGui::BeginTabItem(Lang::instance().get("OPTIMIZER"))) {
                     ImGui::Dummy(ImVec2(0, 15));
                     drawInfoBanner(Lang::instance().get("DESC_OPTIMIZER"));
                     renderOptimizer();
                     ImGui::EndTabItem();
                 }

                 // --- STARTUP ---
                 if (ImGui::BeginTabItem(Lang::instance().get("STARTUP"))) { 
                     ImGui::Dummy(ImVec2(0, 15));
                     drawInfoBanner(Lang::instance().get("DESC_STARTUP"));
                     renderStartupManager();
                     ImGui::EndTabItem();
                 }

                 ImGui::EndTabBar();
             }
             ImGui::PopStyleColor(3);

             ImGui::End(); 
        }

    private:
        std::unique_ptr<Cleaner> cleaner_;
        Cleaner::ScanResult lastScan_ = {0, 0};

        // --- UX COMPONENTS ---
        
        void drawInfoBanner(const char* text) {
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.48f, 0.8f, 0.15f)); 
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.48f, 0.8f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
            
            // Calculate dynamic height
            float wrapWidth = ImGui::GetContentRegionAvail().x - 60.0f; // Account for icon + padding
            ImVec2 textSize = ImGui::CalcTextSize(text, NULL, false, wrapWidth);
            float minHeight = 60.0f;
            float height = textSize.y + 40.0f; // Padding top/bottom
            if (height < minHeight) height = minHeight;

            ImGui::BeginChild("Banner", ImVec2(0, height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            
            ImGui::AlignTextToFramePadding();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (height - textSize.y) / 2.0f - 10.0f); // Vertically center approx
            
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[INFO]");
            ImGui::SameLine();
            ImGui::TextWrapped("%s", text);
            
            ImGui::EndChild();
            
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            ImGui::Dummy(ImVec2(0, 15));
        }

        void HelpMarker(const char* desc) {
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(400.0f);
                ImGui::TextUnformatted(desc);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
        
        void BeginCard(const char* name, float height_factor = 0.0f) {
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f)); 
            
            float h = height_factor > 0 ? ImGui::GetContentRegionAvail().y * height_factor : 0;
            if (height_factor == 0) h = 0; // Auto height (remaining if 0 passed to BeginChild with flow)
            
            ImGui::BeginChild(name, ImVec2(0, h), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
            ImGui::PopStyleColor(); 
            ImGui::PopStyleVar();   
        }

        void EndCard() {
            ImGui::EndChild();
        }

        void renderHeader(double cpu, float ramPercent) {
             ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]); 
             ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "LSAA AGENT");
             ImGui::PopFont();
             
             ImGui::SameLine(); 
             ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), "|");
             ImGui::SameLine(); 
             ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s", Lang::instance().get("SYSTEM_ONLINE"));
             
             // Top Right Stats
             float avail = ImGui::GetContentRegionAvail().x;
             float startX = avail - 600;
             if (startX < 300) startX = 300; 

             ImGui::SameLine(startX);
             
             ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
             if (ImGui::Button(Lang::instance().getLanguage() == Language::EN ? "Lang: EN" : "Lang: FR", ImVec2(100, 32))) {
                 Lang::instance().toggle();
             }
             ImGui::PopStyleVar();

             ImGui::SameLine(); ImGui::Text("|"); // Separator
             
             ImGui::SameLine(); 
             ImGui::Text("CPU: %.1f%%", cpu);
             ImGui::SameLine(startX + 180);
             ImGui::Text("RAM: %.1f%%", ramPercent);
             ImGui::SameLine(startX + 300);
             ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
        }

        void renderDashboard(double cpu, long long ramUsed, long long ramTotal, 
                          const std::vector<ProcessInfo>& topProcesses,
                          const std::vector<std::string>& logs,
                          std::function<void(DWORD)> onKillProcess,
                          std::function<void()> onRunScript,
                          std::function<void()> onSendNotification) 
        {
             float width = ImGui::GetContentRegionAvail().x;
             // float height = ImGui::GetContentRegionAvail().y; (Unused)
             
             float col1W = width * 0.40f;
             if (col1W < 350) col1W = 350;

             float col2W = width - col1W - 20;

             // --- LEFT (METRICS) ---
             ImGui::BeginChild("LeftCol", ImVec2(col1W, 0), false);
                
                BeginCard("MonPanel", 0.0f); 
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", Lang::instance().get("PROCESSOR_LOAD"));
                    HelpMarker(Lang::instance().get("HELP_CPU"));
                    
                    ImGui::Dummy(ImVec2(0, 5));
                    ImGui::ProgressBar((float)(cpu / 100.0), ImVec2(-1, 25)); // Taller bar
                    
                    ImGui::Dummy(ImVec2(0, 5));
                    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 0.6f, 1.0f, 1.0f));
                    ImGui::PlotLines("##CPU", historyCpu_.data(), (int)historyCpu_.size(), 0, NULL, 0.0f, 100.0f, ImVec2(-1, 60));
                    ImGui::PopStyleColor();

                    ImGui::Dummy(ImVec2(0, 20));

                    float ramPercent = (ramTotal > 0) ? (float)((double)ramUsed / ramTotal * 100.0) : 0;
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s (%.2f GB)", Lang::instance().get("MEMORY_USAGE"), ramUsed / 1024.0 / 1024.0 / 1024.0);
                    HelpMarker(Lang::instance().get("HELP_RAM"));

                    ImGui::Dummy(ImVec2(0, 5));
                    ImGui::ProgressBar(ramPercent / 100.0f, ImVec2(-1, 25));
                    
                    ImGui::Dummy(ImVec2(0, 5));
                    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
                    ImGui::PlotLines("##RAM", historyRam_.data(), (int)historyRam_.size(), 0, NULL, 0.0f, 100.0f, ImVec2(-1, 60));
                    ImGui::PopStyleColor();
                }
                EndCard();

                ImGui::Dummy(ImVec2(0, 20));

                BeginCard("ActionPanel", 0.0f);
                {
                    ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "%s", Lang::instance().get("QUICK_ACTIONS"));
                    ImGui::Separator(); ImGui::Dummy(ImVec2(0, 10));
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.35f, 1.0f)); 
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.75f, 0.4f, 1.0f));
                    if (ImGui::Button(Lang::instance().get("RUN_CLEANER"), ImVec2(-1, 50))) if (onRunScript) onRunScript();
                    ImGui::PopStyleColor(2);
                    
                    ImGui::Dummy(ImVec2(0, 10));
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.25f, 0.3f, 1.0f));
                    if (ImGui::Button(Lang::instance().get("TEST_NOTIF"), ImVec2(-1, 50))) if (onSendNotification) onSendNotification();
                    ImGui::PopStyleColor();
                }
                EndCard();

             ImGui::EndChild();
             ImGui::SameLine();

             // --- RIGHT (PROCESS & LOGS) ---
             ImGui::BeginChild("RightCol", ImVec2(col2W, 0), false);
                
                BeginCard("ProcPanel", 0.55f);
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", Lang::instance().get("TOP_PROCESSES"));
                    HelpMarker(Lang::instance().get("TopProcessDesc"));
                    ImGui::Dummy(ImVec2(0, 10));

                    if (ImGui::BeginTable("table_processes", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PadOuterX)) {
                        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn(Lang::instance().get("NAME"), ImGuiTableColumnFlags_WidthStretch); 
                        ImGui::TableSetupColumn("MEM", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                        ImGui::TableSetupColumn("ACT", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableHeadersRow();

                        for (const auto& p : topProcesses) {
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 35.0f); 
                            ImGui::TableNextColumn(); ImGui::TextDisabled("%d", p.pid);
                            ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "%s", p.name.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("%.1f MB", p.memoryBytes / 1024.0 / 1024.0);
                            ImGui::TableNextColumn();
                            
                            ImGui::PushID(p.pid);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.25f, 0.25f, 1.0f));
                            if (ImGui::Button("KILL", ImVec2(-1, 0))) if (onKillProcess) onKillProcess(p.pid);
                            ImGui::PopStyleColor();
                            if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Lang::instance().get("HELP_KILL"));
                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                    }
                }
                EndCard();

                ImGui::Dummy(ImVec2(0, 20));

                BeginCard("LogPanel", 0.0f); 
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", Lang::instance().get("LOGS"));
                    ImGui::Separator();
                    ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                    for (auto it = logs.begin(); it != logs.end(); ++it) {
                        ImVec4 col = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                        if (it->find("[ERROR]") != std::string::npos) col = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                        else if (it->find("[WARN]") != std::string::npos) col = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                        else if (it->find("[INFO]") != std::string::npos) col = ImVec4(0.3f, 0.9f, 0.5f, 1.0f);
                        
                        ImGui::TextColored(col, "%s", it->c_str());
                    }
                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
                    ImGui::EndChild();
                }
                EndCard();

             ImGui::EndChild();
        }

        void renderRulesEditor(std::function<void()> onReloadEngine) {
            auto& config = ConfigManager::instance();
            auto& rules = config.getRules();
            bool changed = false;
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

            ImGui::TextColored(ImVec4(1,1,1,0.8f), "%s", Lang::instance().get("RULES"));
            ImGui::SameLine();
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.48f, 0.8f, 1.0f));
            if (ImGui::Button(Lang::instance().get("SAVE_CONFIG"), ImVec2(220, 45))) {
                config.save();
                if (onReloadEngine) onReloadEngine();
            }
            ImGui::PopStyleColor();
            
            ImGui::PopStyleVar();

            ImGui::Separator(); ImGui::Dummy(ImVec2(0, 20));

            BeginCard("RulesCard");
            if (ImGui::BeginTable("RulesTable", 5, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX)) {
                ImGui::TableSetupColumn(Lang::instance().get("NAME"), ImGuiTableColumnFlags_WidthFixed, 250.0f);
                ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Cond", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Threshold Limit", ImGuiTableColumnFlags_WidthFixed, 300.0f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                
                ImGui::TableHeadersRow();

                for (auto& rule : rules) {
                    ImGui::PushID(rule.name.c_str());
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, 50.0f); 
                    
                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", rule.name.c_str());
                    
                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("%s", rule.metric.c_str());
                    
                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding(); 
                    ImGui::Text(" > ");
                    
                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    float val = (float)rule.threshold;
                    ImGui::SetNextItemWidth(250);
                    if (ImGui::SliderFloat("##thresh", &val, 0.0f, 1000.0f, "%.0f")) {
                        rule.threshold = (double)val;
                        changed = true;
                    }

                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    if (ImGui::Checkbox("Enabled", &rule.enabled)) changed = true;
                    
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            EndCard();
        }

        void renderOptimizer() {
            static CleanParams params; 
            ImGui::TextColored(ImVec4(1,1,1,0.8f), "System Cleaner (Junk Files)");
            ImGui::Separator(); ImGui::Dummy(ImVec2(0, 20));

            BeginCard("CleanCard", 0.0f);
            
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Targets:");
            ImGui::Dummy(ImVec2(0, 10));

            ImGui::Checkbox(Lang::instance().get("SYS_TEMP"), &params.sysTemp); 
            ImGui::SameLine(300);
            ImGui::TextDisabled("(%s)", cleaner_->getTempPath().c_str());

            ImGui::Checkbox(Lang::instance().get("CHROME_CACHE"), &params.chrome);
            ImGui::Checkbox(Lang::instance().get("EDGE_CACHE"), &params.edge);
            ImGui::Checkbox(Lang::instance().get("FIREFOX_CACHE"), &params.firefox);
            ImGui::Checkbox(Lang::instance().get("DNS_CACHE"), &params.dns);

            ImGui::Dummy(ImVec2(0, 30));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.7f, 1.0f));
            if (ImGui::Button(Lang::instance().get("SCAN_NOW"), ImVec2(300, 60))) {
                lastScan_ = cleaner_->scan(params);
            }
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0, 20));

            if (lastScan_.fileCount > 0) {
                 char buf[256];
                 snprintf(buf, 256, Lang::instance().get("FOUND_FILES"), lastScan_.fileCount, lastScan_.totalSize / 1024.0 / 1024.0);
                 ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "%s", buf);
                 
                 ImGui::Dummy(ImVec2(0, 20));
                 ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                 if (ImGui::Button(Lang::instance().get("CLEAN_ALL"), ImVec2(300, 60))) {
                     cleaner_->clean(params);
                     lastScan_ = cleaner_->scan(params); 
                 }
                 ImGui::PopStyleColor();
            } else {
                ImGui::TextDisabled("%s", Lang::instance().get("NO_JUNK"));
            }
            EndCard();
        }

        void renderStartupManager() {
            ImGui::Text("%s", Lang::instance().get("STARTUP_MANAGER"));
            ImGui::Separator(); ImGui::Dummy(ImVec2(0, 20));

            BeginCard("StartupList", 0.6f);
            
            static std::vector<StartupItem> items = StartupManager::getStartupItems();
            
            if (ImGui::Button("Refresh List", ImVec2(120, 35))) items = StartupManager::getStartupItems();
            ImGui::Dummy(ImVec2(0, 20));

             // Use columns but styled better
             if (ImGui::BeginTable("StartupTable", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn(Lang::instance().get("NAME"), ImGuiTableColumnFlags_WidthFixed, 250.0f);
                ImGui::TableSetupColumn(Lang::instance().get("PATH"), ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn(Lang::instance().get("ACTION"), ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableHeadersRow();

                for (auto& item : items) {
                     ImGui::PushID(item.name.c_str());
                     ImGui::TableNextRow(ImGuiTableRowFlags_None, 45.0f);
                     
                     ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                     ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "%s", item.name.c_str());
                     
                     ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                     ImGui::TextDisabled("%s", item.path.c_str());
                     
                     ImGui::TableNextColumn();
                     // Use a toggle style button
                     ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.6f));
                     if (ImGui::Button(Lang::instance().get("REMOVE"), ImVec2(120, 32))) {
                         if (StartupManager::removeItem(item.name)) items = StartupManager::getStartupItems();
                     }
                     ImGui::PopStyleColor();
                     
                     ImGui::PopID();
                }
                ImGui::EndTable();
             }
             EndCard();

             ImGui::Dummy(ImVec2(0, 20));
             
             BeginCard("AddManual");
             ImGui::Text("Add New Item:");
             ImGui::Dummy(ImVec2(0, 10));
             
             static char newName[128] = "";
             static char newPath[512] = "";
             
             ImGui::PushItemWidth(350);
             ImGui::InputText(Lang::instance().get("NAME"), newName, 128);
             ImGui::SameLine();
             ImGui::InputText(Lang::instance().get("PATH"), newPath, 512);
             ImGui::PopItemWidth();
             
             ImGui::Dummy(ImVec2(0, 20));
             
             ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
             if (ImGui::Button(Lang::instance().get("ADD"), ImVec2(180, 40))) {
                 if (strlen(newName) > 0 && strlen(newPath) > 0) {
                     StartupManager::addItem(newName, newPath);
                     items = StartupManager::getStartupItems(); // Refresh
                     memset(newName, 0, 128);
                     memset(newPath, 0, 512);
                 }
             }
             ImGui::PopStyleColor();
             EndCard();
        }

        void renderServiceManager() {
            ImGui::Text("%s", Lang::instance().get("SERVICES"));
            ImGui::Separator(); ImGui::Dummy(ImVec2(0, 20));

            BeginCard("ServicesList", 0.0f); // Auto height
            
            static std::vector<ServiceInfo> services = ServiceManager::getServices();
            static std::string filter = "";

            if (ImGui::Button("Refresh Services", ImVec2(150, 35))) {
                services = ServiceManager::getServices();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(300);
            static char searchBuf[128] = "";
            if (ImGui::InputTextWithHint("##search", "Search Services...", searchBuf, 128)) {
                filter = std::string(searchBuf);
            }

            ImGui::Dummy(ImVec2(0, 20));

            if (ImGui::BeginTable("ServicesTable", 5, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn(Lang::instance().get("NAME"), ImGuiTableColumnFlags_WidthFixed, 200.0f);
                ImGui::TableSetupColumn(Lang::instance().get("DISPLAY_NAME"), ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn(Lang::instance().get("PID"), ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn(Lang::instance().get("STATUS"), ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn(Lang::instance().get("ACTION"), ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                for (auto& svc : services) {
                    // Filter logic
                    if (!filter.empty()) {
                        std::string lowerName = svc.name;
                        std::string lowerDisplay = svc.displayName;
                        std::string lowerFilter = filter;
                        // Basic lowercase conversion for search (not robust for UTF8 but ok here)
                        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                        std::transform(lowerDisplay.begin(), lowerDisplay.end(), lowerDisplay.begin(), ::tolower);
                        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
                        
                        if (lowerName.find(lowerFilter) == std::string::npos && 
                            lowerDisplay.find(lowerFilter) == std::string::npos) {
                            continue;
                        }
                    }

                    ImGui::PushID(svc.name.c_str());
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, 40.0f);

                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "%s", svc.name.c_str());

                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", svc.displayName.c_str());

                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("%d", svc.pid);

                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    if (svc.stateCode == SERVICE_RUNNING) 
                        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", svc.status.c_str());
                    else if (svc.stateCode == SERVICE_STOPPED)
                        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "%s", svc.status.c_str());
                    else 
                        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "%s", svc.status.c_str());

                    ImGui::TableNextColumn();
                    if (svc.canStop) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.25f, 0.25f, 0.6f));
                        if (ImGui::Button(Lang::instance().get("STOP_SERVICE"), ImVec2(100, 30))) {
                            if (ServiceManager::stopService(svc.name)) {
                                svc.status = "Stopping..."; // Optimistic update
                                svc.canStop = false;
                            }
                        }
                        ImGui::PopStyleColor();
                    } else if (svc.stateCode == SERVICE_STOPPED) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.8f, 0.25f, 0.6f));
                        if (ImGui::Button(Lang::instance().get("START_SERVICE"), ImVec2(100, 30))) {
                             if (ServiceManager::startService(svc.name)) {
                                svc.status = "Starting...";
                             }
                        }
                        ImGui::PopStyleColor();
                    }

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
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

        void applyProTheme() {
            ImGuiStyle& style = ImGui::GetStyle();
            
            style.WindowRounding = 8.0f;
            style.ChildRounding = 10.0f;
            style.FrameRounding = 6.0f;
            style.GrabRounding = 4.0f;
            style.PopupRounding = 6.0f;
            style.TabRounding = 6.0f;

            style.ItemSpacing = ImVec2(16, 16);
            style.FramePadding = ImVec2(12, 10);
            style.WindowPadding = ImVec2(24, 24);
            style.ScrollbarSize = 12.0f;
            style.ScrollbarRounding = 12.0f;
            
            // Darker, cleaner theme
            ImVec4 bg             = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
            ImVec4 panel          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
            ImVec4 panelHover     = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
            
            ImVec4 primary        = ImVec4(0.00f, 0.52f, 0.85f, 1.00f); 
            ImVec4 primaryActive  = ImVec4(0.00f, 0.60f, 0.95f, 1.00f);

            ImVec4 text           = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
            ImVec4 textDisabled   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

            style.Colors[ImGuiCol_Text] = text;
            style.Colors[ImGuiCol_TextDisabled] = textDisabled;
            style.Colors[ImGuiCol_WindowBg] = bg;
            style.Colors[ImGuiCol_ChildBg] = panel;
            style.Colors[ImGuiCol_PopupBg] = panel;
            
            style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // No borders mostly
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.03f, 0.03f, 0.04f, 1.0f);
            style.Colors[ImGuiCol_FrameBgHovered] = panelHover;
            style.Colors[ImGuiCol_FrameBgActive] = primary;
            
            style.Colors[ImGuiCol_TitleBg] = bg;
            
            style.Colors[ImGuiCol_CheckMark] = primaryActive;
            style.Colors[ImGuiCol_SliderGrab] = primary;
            style.Colors[ImGuiCol_SliderGrabActive] = primaryActive;
            
            style.Colors[ImGuiCol_Button] = panelHover;
            style.Colors[ImGuiCol_ButtonHovered] = primary;
            style.Colors[ImGuiCol_ButtonActive] = primaryActive;
            
            style.Colors[ImGuiCol_Header] = panelHover;
            style.Colors[ImGuiCol_HeaderHovered] = panelHover;
            style.Colors[ImGuiCol_HeaderActive] = panel;
            
            style.Colors[ImGuiCol_PlotLines] = primary;
            style.Colors[ImGuiCol_PlotLinesHovered] = primaryActive;
        }

        void loadFonts() {
            ImGuiIO& io = ImGui::GetIO();
             if (std::filesystem::exists("C:\\Windows\\Fonts\\segoeui.ttf")) {
                io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 22.0f); 
                io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 34.0f); // Title Font
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
        std::vector<float> historyCpu_;
        std::vector<float> historyRam_;
    };
}
