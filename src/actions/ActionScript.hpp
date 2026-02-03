#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "../engine/Rule.hpp"

namespace lsaa {

    class ActionScript : public IAction {
    public:
        ActionScript(std::string command) : command_(std::move(command)) {}

        void execute() override {
            STARTUPINFOA si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            std::vector<char> cmd(command_.begin(), command_.end());
            cmd.push_back(0);

            if (CreateProcessA(NULL, 
                cmd.data(), 
                NULL, 
                NULL, 
                FALSE, 
                0, 
                NULL, 
                NULL, 
                &si, 
                &pi)
            ) {
                LSAA_LOG_INFO("ActionScript Executed: " + command_);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            } else {
                LSAA_LOG_ERROR("ActionScript Failed: " + command_);
            }
        }

        std::string getName() const override { return "ActionScript: " + command_; }

    private:
        std::string command_;
    };

}
