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

namespace lsaa {

    class GuiManager {
    public:
        static GuiManager& instance() {
            static GuiManager instance;
            return instance;
        }

        bool init() {
            // Setup window
            if (!glfwInit()) return false;

            // GL 3.0 + GLSL 130
            const char* glsl_version = "#version 130";
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

            // Create window with graphics context
            window_ = glfwCreateWindow(1280, 800, "LSAA - System Automation Agent", NULL, NULL);
            if (window_ == NULL) return false;
            
            glfwMakeContextCurrent(window_);
            glfwSwapInterval(1); // Enable vsync

            // setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            
            // Setup Fonts
            loadFonts();
            
            // Setup Dear ImGui style
            embraceTheDarkness(); // Custom Style

            // Setup Platform/Renderer backends
            ImGui_ImplGlfw_InitForOpenGL(window_, true);
            ImGui_ImplOpenGL3_Init(glsl_version);

            return true;
        }

        bool shouldClose() {
            return glfwWindowShouldClose(window_);
        }

        void beginFrame() {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        void endFrame() {
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.12f, 1.0f); // Match background
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

        void drawDashboard(double cpu, long long ramUsed, long long ramTotal, 
                          const std::vector<ProcessInfo>& topProcesses,
                          const std::vector<std::string>& logs,
                          std::function<void(DWORD)> onKillProcess,
                          std::function<void()> onRunScript,
                          std::function<void()> onSendNotification) {
             
             // Update history
             if (cpuHistory_.size() > 120) cpuHistory_.erase(cpuHistory_.begin());
             cpuHistory_.push_back((float)cpu);

             float ramPercent = 0.0f;
             if (ramTotal > 0) ramPercent = (float)((double)ramUsed / (double)ramTotal * 100.0);
             
             if (ramHistory_.size() > 120) ramHistory_.erase(ramHistory_.begin());
             ramHistory_.push_back(ramPercent);

             // Define Layout
             ImGuiViewport* viewport = ImGui::GetMainViewport();
             ImVec2 workPos = viewport->WorkPos;
             ImVec2 workSize = viewport->WorkSize;
             float pad = 10.0f;
             
             // 1. TOP BAR (Status)
             ImGui::SetNextWindowPos(ImVec2(workPos.x + pad, workPos.y + pad));
             ImGui::SetNextWindowSize(ImVec2(workSize.x - 2*pad, 60));
             ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
             ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.4f, 1.0f), "LSAA CORE"); 
             ImGui::SameLine(); ImGui::Text("| SYSTEM ONLINE");
             ImGui::SameLine(300);
             ImGui::Text("CPU: %.1f%%", cpu);
             ImGui::SameLine(450);
             ImGui::Text("RAM: %.1f%%", ramPercent);
             ImGui::SameLine(600);
             ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
             ImGui::End();

             // 2. LEFT PANEL (Graphs & Controls)
             float leftWidth = workSize.x * 0.45f - pad; // Slightly wider
             float bottomHeight = workSize.y - 80 - 2*pad;
             
             ImGui::SetNextWindowPos(ImVec2(workPos.x + pad, workPos.y + 80));
             ImGui::SetNextWindowSize(ImVec2(leftWidth, bottomHeight));
             // Added NoTitleBar
             ImGui::Begin("Monitoring", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar); 
             
             ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "PROCESSOR LOAD");
             ImGui::ProgressBar(cpu / 100.0f, ImVec2(-1, 2)); // Tiny clean bar
             ImGui::PlotLines("##CPU", cpuHistory_.data(), (int)cpuHistory_.size(), 0, NULL, 0.0f, 100.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100));
             
             ImGui::Spacing(); ImGui::Spacing();
             
             ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "MEMORY USAGE (%.2f / %.2f GB)", ramUsed / 1024.0 / 1024.0 / 1024.0, ramTotal / 1024.0 / 1024.0 / 1024.0);
             ImGui::ProgressBar(ramPercent / 100.0f, ImVec2(-1, 2));
             ImGui::PlotLines("##RAM", ramHistory_.data(), (int)ramHistory_.size(), 0, NULL, 0.0f, 100.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100));
             
             ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

             ImGui::TextColored(ImVec4(1, 1, 1, 0.8f), "QUICK ACTIONS");
             if (ImGui::Button("RUN CLEAN SCRIPT", ImVec2(-1, 40))) {
                 if (onRunScript) onRunScript();
             }
             ImGui::Spacing();
             if (ImGui::Button("TEST NOTIFICATION", ImVec2(-1, 40))) {
                 if (onSendNotification) onSendNotification();
             }
             ImGui::End();

             // 3. RIGHT PANEL (Process List & Logs)
             float rightPos = workPos.x + leftWidth + 2*pad;
             float rightWidth = workSize.x - leftWidth - 3*pad;
             
             ImGui::SetNextWindowPos(ImVec2(rightPos, workPos.y + 80));
             ImGui::SetNextWindowSize(ImVec2(rightWidth, bottomHeight * 0.65f));
             // Added NoTitleBar
             ImGui::Begin("Top Processes", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
             
             ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "TOP MEMORY CONSUMERS");
             if (ImGui::BeginTable("table_processes", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PadOuterX)) {
                 ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                 ImGui::TableSetupColumn("NAME");
                 ImGui::TableSetupColumn("MEMORY", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                 ImGui::TableSetupColumn("ACTION", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                 ImGui::TableHeadersRow();

                 for (const auto& p : topProcesses) {
                     ImGui::TableNextRow();
                     ImGui::TableNextColumn(); ImGui::Text("%d", p.pid);
                     ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "%s", p.name.c_str());
                     ImGui::TableNextColumn(); ImGui::Text("%.1f", p.memoryBytes / 1024.0 / 1024.0);
                     ImGui::TableNextColumn();
                     
                     ImGui::PushID(p.pid);
                     ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                     if (ImGui::Button("KILL", ImVec2(50, 0))) {
                         if (onKillProcess) onKillProcess(p.pid);
                     }
                     ImGui::PopStyleColor();
                     ImGui::PopID();
                 }
                 ImGui::EndTable();
             }
             ImGui::End();

             // 4. BOTTOM RIGHT (Logs)
             ImGui::SetNextWindowPos(ImVec2(rightPos, workPos.y + 80 + bottomHeight * 0.65f + pad)); // Adjusted Offset
             ImGui::SetNextWindowSize(ImVec2(rightWidth, bottomHeight * 0.35f - pad));
             ImGui::Begin("System Logs", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
             
             ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "SYSTEM EVENT LOG");
             ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
             for (auto it = logs.begin(); it != logs.end(); ++it) { // Show in order
                 if (it->find("[ERROR]") != std::string::npos) ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), " >> %s", it->c_str());
                 else if (it->find("[WARN]") != std::string::npos) ImGui::TextColored(ImVec4(1,0.7f,0.1f,1), " >> %s", it->c_str());
                 else ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), " %s", it->c_str());
             }
             if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                 ImGui::SetScrollHereY(1.0f);
             ImGui::EndChild();
             
             ImGui::End();
        }

    private:
        GuiManager() {
            cpuHistory_.reserve(120);
            ramHistory_.reserve(120);
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

        // Font Loading Helper
        void loadFonts() {
            ImGuiIO& io = ImGui::GetIO();
            // Try load Segoe UI (Standard Windows Font)
            // Size: 20.0f (High DPI friendly-ish)
             if (std::filesystem::exists("C:\\Windows\\Fonts\\segoeui.ttf")) {
                io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f);
             } else {
                 // Fallback
                 io.Fonts->AddFontDefault();
             }
        }


        ~GuiManager() = default;

        GLFWwindow* window_ = nullptr;
        std::vector<float> cpuHistory_;
        std::vector<float> ramHistory_;
    };
}
