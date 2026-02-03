#pragma once
#include "../core/IMonitor.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <mutex>

namespace lsaa {

    struct ProcessInfo {
        DWORD pid;
        std::string name;
        SIZE_T memoryBytes;
    };

    class ProcessMonitor : public IMonitor {
    public:
        std::string getName() const override { return "ProcessMonitor"; }

        bool initialize() override {
            return true;
        }

        bool collect() override {
            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnapshot == INVALID_HANDLE_VALUE) return false;

            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            if (!Process32First(hSnapshot, &pe32)) {
                CloseHandle(hSnapshot);
                return false;
            }

            std::vector<ProcessInfo> processes;
            do {
                SIZE_T memUsage = 0;
                // Get memory usage
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        memUsage = pmc.WorkingSetSize;
                    }
                    CloseHandle(hProcess);
                }

                processes.push_back({
                    pe32.th32ProcessID,
                    pe32.szExeFile,
                    memUsage
                });

            } while (Process32Next(hSnapshot, &pe32));

            CloseHandle(hSnapshot);

             processCount_ = processes.size();

            // Sort by Memory Descending
            if (!processes.empty()) {
                std::sort(processes.begin(), processes.end(), 
                    [](const ProcessInfo& a, const ProcessInfo& b) {
                        return a.memoryBytes > b.memoryBytes; // Descending
                    });
                
                std::lock_guard<std::mutex> lock(mutex_);
                topProcesses_.clear();
                // FIX: use (std::min) to avoid macro conflict
                size_t limit = (std::min)((size_t)5, processes.size());
                for(size_t i=0; i<limit; ++i) {
                    topProcesses_.push_back(processes[i]);
                }

                topProcessName_ = processes[0].name;
                topProcessMem_ = processes[0].memoryBytes;
            } else {
                std::lock_guard<std::mutex> lock(mutex_);
                topProcessName_ = "None";
                topProcessMem_ = 0;
                topProcesses_.clear();
            }
            
            return true;
        }

        MetricsMap getMetrics() const override {
            std::lock_guard<std::mutex> lock(mutex_);
            return {
                {"process_count", (long long)processCount_},
                {"top_mem_process_name", topProcessName_},
                {"top_mem_bytes", (long long)topProcessMem_}
            };
        }

        std::vector<ProcessInfo> getTopProcesses() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return topProcesses_;
        }

    private:
        size_t processCount_ = 0;
        std::string topProcessName_;
        SIZE_T topProcessMem_ = 0;
        std::vector<ProcessInfo> topProcesses_;
        mutable std::mutex mutex_;
    };
}
