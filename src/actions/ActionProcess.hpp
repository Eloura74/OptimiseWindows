#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include "../engine/Rule.hpp"

namespace lsaa {

    class ActionKillProcess : public IAction {
    public:
        ActionKillProcess(std::string processName) : processName_(std::move(processName)), targetPid_(0) {}
        ActionKillProcess(DWORD pid) : processName_("PID:" + std::to_string(pid)), targetPid_(pid) {}

        void execute() override {
            if (targetPid_ != 0) {
                 terminatePid(targetPid_);
                 return;
            }

            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnapshot == INVALID_HANDLE_VALUE) return;

            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            if (Process32First(hSnapshot, &pe32)) {
                do {
                    if (processName_ == pe32.szExeFile) {
                        terminatePid(pe32.th32ProcessID);
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }

            CloseHandle(hSnapshot);
        }

        std::string getName() const override { return "ActionKillProcess: " + processName_; }

    private:
        std::string processName_;
        DWORD targetPid_;

        void terminatePid(DWORD pid) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProcess) {
                if (TerminateProcess(hProcess, 1)) {
                     LSAA_LOG_WARN("ActionKillProcess: Terminated " + processName_ + " (PID: " + std::to_string(pid) + ")");
                } else {
                     LSAA_LOG_ERROR("ActionKillProcess: Failed to terminate " + processName_ + " (PID: " + std::to_string(pid) + ")");
                }
                CloseHandle(hProcess);
            }
        }
    };

}
