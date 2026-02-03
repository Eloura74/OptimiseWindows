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
            window_ = glfwCreateWindow(1280, 800, "LSAA - System Automation Agent", NULL, NULL);
            if (window_ == NULL) return false;
            glfwMakeContextCurrent(window_);
            glfwSwapInterval(1); 
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            loadFonts();
            embraceTheDarkness(); 
            ImGui_ImplGlfw_InitForOpenGL(window_, true);
            ImGui_ImplOpenGL3_Init(glsl_version);
            cleaner_ = std::make_unique<Cleaner>();
            return true;
        }

        bool shouldClose() { return glfwWindowShouldClose(window_); }
        void beginFrame() { glfwPollEvents(); ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame(); }
        void endFrame() { 
            ImGui::Render(); int w, h; glfwGetFramebufferSize(window_, &w, &h); glViewport(0, 0, w, h); 
            glClearColor(0.10f, 0.11f, 0.13f, 1.0f); glClear(GL_COLOR_BUFFER_BIT); 
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); glfwSwapBuffers(window_); 
        }
        void cleanup() { ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext(); glfwDestroyWindow(window_); glfwTerminate(); }

        // --- MAIN RENDER LOOP ---
        void drawUI(double cpu, long long ramUsed, long long ramTotal, 
                          const std::vector<ProcessInfo>& topProcesses,
                          const std::vector<std::string>& logs,
                          std::function<void(DWORD)> onKillProcess,
                          std::function<void()> onRunScript,
                          std::function<void()> onSendNotification,
                          std::function<void()> onReloadEngine) // CALLBACK #9
        {
             updateHistory(cpu, ramUsed, ramTotal);

             const ImGuiViewport* viewport = ImGui::GetMainViewport();
             ImGui::SetNextWindowPos(viewport->WorkPos);
             ImGui::SetNextWindowSize(viewport->WorkSize);
             ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
             ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
             ImGui::Begin("MainDock", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
             ImGui::PopStyleVar(2);

             renderHeader(cpu, historyRam_.empty() ? 0 : historyRam_.back());

             ImGui::Separator();

             // TABS
             if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {
                 
                 if (ImGui::BeginTabItem(Lang::instance().get("DASHBOARD"))) {
                     renderDashboard(cpu, ramUsed, ramTotal, topProcesses, logs, onKillProcess, onRunScript, onSendNotification);
                     ImGui::EndTabItem();
                 }

                 if (ImGui::BeginTabItem(Lang::instance().get("RULES"))) {
                     renderRulesEditor(onReloadEngine);
                     ImGui::EndTabItem();
                 }

                 if (ImGui::BeginTabItem(Lang::instance().get("OPTIMIZER"))) {
                     renderOptimizer();
                     ImGui::EndTabItem();
                 }

                 if (ImGui::BeginTabItem(Lang::instance().get("STARTUP"))) { 
                     renderStartupManager();
                     ImGui::EndTabItem();
                 }

                 ImGui::EndTabBar();
             }

             ImGui::End(); 
        }

    private:
        std::unique_ptr<Cleaner> cleaner_;
        Cleaner::ScanResult lastScan_ = {0, 0};
        
        void renderHeader(double cpu, float ramPercent) {
             ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.4f, 1.0f), "LSAA CORE"); 
             ImGui::SameLine(); ImGui::Text("%s", Lang::instance().get("SYSTEM_ONLINE"));
             
             // Language Switcher
             ImGui::SameLine(300);
             if (ImGui::Button(Lang::instance().getLanguage() == Language::EN ? "EN" : "FR", ImVec2(40, 0))) {
                 Lang::instance().toggle();
             }

             float width = ImGui::GetContentRegionAvail().x;
             ImGui::SameLine(width - 400);
             ImGui::Text("CPU: %.1f%%", cpu);
             ImGui::SameLine(width - 250);
             ImGui::Text("RAM: %.1f%%", ramPercent);
             ImGui::SameLine(width - 100);
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
             float height = ImGui::GetContentRegionAvail().y;
             float col1W = width * 0.45f;
             float col2W = width - col1W - 20;

             ImGui::BeginChild("LeftCol", ImVec2(col1W, height), false);
                
                ImGui::BeginChild("MonPanel", ImVec2(0, height * 0.6f), true);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "%s", Lang::instance().get("PROCESSOR_LOAD"));
                ImGui::ProgressBar((float)(cpu / 100.0), ImVec2(-1, 4));
                ImGui::PlotLines("##CPU", historyCpu_.data(), (int)historyCpu_.size(), 0, NULL, 0.0f, 100.0f, ImVec2(-1, 80));

                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                float ramPercent = (ramTotal > 0) ? (float)((double)ramUsed / ramTotal * 100.0) : 0;
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "%s (%.2f GB)", Lang::instance().get("MEMORY_USAGE"), ramUsed / 1024.0 / 1024.0 / 1024.0);
                ImGui::ProgressBar(ramPercent / 100.0f, ImVec2(-1, 4));
                ImGui::PlotLines("##RAM", historyRam_.data(), (int)historyRam_.size(), 0, NULL, 0.0f, 100.0f, ImVec2(-1, 80));
                ImGui::EndChild();

                ImGui::BeginChild("ActionPanel", ImVec2(0, 0), true);
                ImGui::TextColored(ImVec4(1, 1, 1, 0.8f), "%s", Lang::instance().get("QUICK_ACTIONS"));
                if (ImGui::Button(Lang::instance().get("RUN_CLEANER"), ImVec2(-1, 40))) if (onRunScript) onRunScript();
                ImGui::Spacing();
                if (ImGui::Button(Lang::instance().get("TEST_NOTIF"), ImVec2(-1, 40))) if (onSendNotification) onSendNotification();
                ImGui::EndChild();

             ImGui::EndChild();
             ImGui::SameLine();

             ImGui::BeginChild("RightCol", ImVec2(col2W, height), false);
                
                ImGui::BeginChild("ProcPanel", ImVec2(0, height * 0.65f), true);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "%s", Lang::instance().get("TOP_PROCESSES"));
                if (ImGui::BeginTable("table_processes", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PadOuterX)) {
                    ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn(Lang::instance().get("NAME"));
                    ImGui::TableSetupColumn("MEM", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("ACT", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                    ImGui::TableHeadersRow();

                    for (const auto& p : topProcesses) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%d", p.pid);
                        ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "%s", p.name.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%.1f", p.memoryBytes / 1024.0 / 1024.0);
                        ImGui::TableNextColumn();
                        ImGui::PushID(p.pid);
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("KILL", ImVec2(50, 0))) if (onKillProcess) onKillProcess(p.pid);
                        ImGui::PopStyleColor();
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::EndChild();

                ImGui::BeginChild("LogPanel", ImVec2(0, 0), true);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "%s", Lang::instance().get("LOGS"));
                ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                for (auto it = logs.begin(); it != logs.end(); ++it) {
                    if (it->find("[ERROR]") != std::string::npos) ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), " >> %s", it->c_str());
                    else if (it->find("[WARN]") != std::string::npos) ImGui::TextColored(ImVec4(1,0.7f,0.1f,1), " >> %s", it->c_str());
                    else ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), " %s", it->c_str());
                }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
                ImGui::EndChild();
                ImGui::EndChild();

             ImGui::EndChild();
        }

        void renderRulesEditor(std::function<void()> onReloadEngine) {
            auto& config = ConfigManager::instance();
            auto& rules = config.getRules();
            bool changed = false;

            ImGui::Text("Monitoring Rules Configuration");
            ImGui::SameLine();
            if (ImGui::Button(Lang::instance().get("SAVE_CONFIG"))) {
                config.save();
                if (onReloadEngine) {
                    onReloadEngine(); // Call Engine Hot Reload
                }
            }

            ImGui::Separator();

            if (ImGui::BeginTable("RulesTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn(Lang::instance().get("NAME"));
                ImGui::TableSetupColumn("Metric");
                ImGui::TableSetupColumn("Condition");
                ImGui::TableSetupColumn("Threshold");
                ImGui::TableSetupColumn("Status");
                ImGui::TableHeadersRow();

                for (auto& rule : rules) {
                    ImGui::PushID(rule.name.c_str());
                    ImGui::TableNextRow();
                    
                    ImGui::TableNextColumn(); ImGui::Text("%s", rule.name.c_str());
                    ImGui::TableNextColumn(); ImGui::TextDisabled("%s", rule.metric.c_str());
                    ImGui::TableNextColumn(); ImGui::Text(" > ");
                    ImGui::TableNextColumn();
                    float val = (float)rule.threshold;
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::SliderFloat("##thresh", &val, 0.0f, 1000.0f)) {
                        rule.threshold = (double)val;
                        changed = true;
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("Enabled", &rule.enabled)) changed = true;
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }

        void renderOptimizer() {
            ImGui::Text("System Cleaner (Junk Files)");
            ImGui::Separator(); ImGui::Spacing();

            ImGui::Text("Target: %s", cleaner_->getTempPath().c_str());

            ImGui::Spacing();
            if (ImGui::Button(Lang::instance().get("SCAN_NOW"), ImVec2(200, 50))) {
                lastScan_ = cleaner_->scan();
            }

            ImGui::Spacing();
            if (lastScan_.fileCount > 0) {
                 char buf[256];
                 snprintf(buf, 256, Lang::instance().get("FOUND_FILES"), lastScan_.fileCount, lastScan_.totalSize / 1024.0 / 1024.0);
                 ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "%s", buf);
                 
                 ImGui::Spacing();
                 if (ImGui::Button(Lang::instance().get("CLEAN_ALL"), ImVec2(200, 50))) {
                     cleaner_->clean();
                     lastScan_ = cleaner_->scan(); 
                 }
            } else {
                ImGui::TextDisabled("%s", Lang::instance().get("NO_JUNK"));
            }
        }

        void renderStartupManager() {
            ImGui::Text("%s", Lang::instance().get("STARTUP_MANAGER"));
            ImGui::Separator();

            static std::vector<StartupItem> items = StartupManager::getStartupItems();
            if (ImGui::Button("Refresh List")) {
                items = StartupManager::getStartupItems();
            }
            
            ImGui::Spacing();

             if (ImGui::BeginTable("StartupTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 400))) {
                ImGui::TableSetupColumn(Lang::instance().get("NAME"), ImGuiTableColumnFlags_WidthFixed, 200.0f);
                ImGui::TableSetupColumn(Lang::instance().get("PATH"));
                ImGui::TableSetupColumn(Lang::instance().get("ACTION"), ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();

                for (auto& item : items) {
                     ImGui::PushID(item.name.c_str());
                     ImGui::TableNextRow();
                     ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", item.name.c_str());
                     ImGui::TableNextColumn(); ImGui::TextDisabled("%s", item.path.c_str());
                     ImGui::TableNextColumn();
                     if (ImGui::Button(Lang::instance().get("REMOVE"), ImVec2(90, 0))) {
                         if (StartupManager::removeItem(item.name)) {
                             items = StartupManager::getStartupItems(); // Refresh
                         }
                     }
                     ImGui::PopID();
                }
                ImGui::EndTable();
             }

             ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
             
             // Simple Add
             ImGui::Text("Add New Item:");
             static char newName[128] = "";
             static char newPath[512] = "";
             ImGui::InputText("Name##new", newName, 128);
             ImGui::InputText("Path##new", newPath, 512);
             if (ImGui::Button(Lang::instance().get("ADD"), ImVec2(100, 0))) {
                 if (strlen(newName) > 0 && strlen(newPath) > 0) {
                     StartupManager::addItem(newName, newPath);
                     items = StartupManager::getStartupItems(); // Refresh
                     memset(newName, 0, 128);
                     memset(newPath, 0, 512);
                 }
             }
        }

        void updateHistory(double cpu, long long ramUsed, long long ramTotal) {
             if (historyCpu_.size() > 120) historyCpu_.erase(historyCpu_.begin());
             historyCpu_.push_back((float)cpu);

             float ramPercent = 0.0f;
             if (ramTotal > 0) ramPercent = (float)((double)ramUsed / (double)ramTotal * 100.0);
             
             if (historyRam_.size() > 120) historyRam_.erase(historyRam_.begin());
             historyRam_.push_back(ramPercent);
        }

        void embraceTheDarkness() {
            ImGuiStyle& style = ImGui::GetStyle();
            
            // Modern Rounding
            style.WindowRounding = 8.0f;
            style.ChildRounding = 8.0f;
            style.FrameRounding = 6.0f;
            style.GrabRounding = 6.0f;
            style.PopupRounding = 6.0f;
            style.ScrollbarRounding = 6.0f;

            // Spacing
            style.ItemSpacing = ImVec2(10, 10);
            style.FramePadding = ImVec2(10, 6);
            style.WindowPadding = ImVec2(16, 16);
            
            // Deep Dark Flat Theme
            ImVec4 bg             = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
            ImVec4 panel          = ImVec4(0.16f, 0.18f, 0.21f, 1.00f);
            ImVec4 panelHover     = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
            ImVec4 primary        = ImVec4(0.00f, 0.48f, 1.00f, 1.00f); // Bright Blue
            ImVec4 primaryHover   = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
            ImVec4 text           = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
            ImVec4 textDisabled   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

            style.Colors[ImGuiCol_Text] = text;
            style.Colors[ImGuiCol_TextDisabled] = textDisabled;
            style.Colors[ImGuiCol_WindowBg] = bg;
            style.Colors[ImGuiCol_ChildBg] = panel;
            style.Colors[ImGuiCol_PopupBg] = panel;
            style.Colors[ImGuiCol_Border] = ImVec4(0,0,0,0);
            style.Colors[ImGuiCol_FrameBg] = panel;
            style.Colors[ImGuiCol_FrameBgHovered] = panelHover;
            style.Colors[ImGuiCol_FrameBgActive] = primary;
            
            style.Colors[ImGuiCol_TitleBg] = bg;
            style.Colors[ImGuiCol_TitleBgActive] = bg;
            style.Colors[ImGuiCol_TitleBgCollapsed] = bg;
            
            style.Colors[ImGuiCol_Button] = panelHover;
            style.Colors[ImGuiCol_ButtonHovered] = primary;
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.40f, 0.90f, 1.00f);
            
            style.Colors[ImGuiCol_Header] = panelHover;
            style.Colors[ImGuiCol_HeaderHovered] = primary;
            style.Colors[ImGuiCol_HeaderActive] = primary;
            
            style.Colors[ImGuiCol_Separator] = panelHover;
            
            style.Colors[ImGuiCol_PlotLines] = primary;
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.5f, 0.2f, 1.0f);
        }

        void loadFonts() {
            ImGuiIO& io = ImGui::GetIO();
             if (std::filesystem::exists("C:\\Windows\\Fonts\\segoeui.ttf")) {
                io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f);
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
