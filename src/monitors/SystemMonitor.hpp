#pragma once
#include "../core/IMonitor.hpp"
#include <windows.h>
#include <vector>
#include <memory> 

namespace lsaa {

    class CpuMonitor : public IMonitor {
    public:
        std::string getName() const override { return "SystemMonitorCPU"; }

        bool initialize() override {
            // Premier appel pour initialiser les temps
            GetSystemTimes(&idleTime_, &kernelTime_, &userTime_);
            return true;
        }

        bool collect() override {
            FILETIME newIdle, newKernel, newUser;
            GetSystemTimes(&newIdle, &newKernel, &newUser);

            ULONGLONG idle = u64(newIdle) - u64(idleTime_);
            ULONGLONG kernel = u64(newKernel) - u64(kernelTime_);
            ULONGLONG user = u64(newUser) - u64(userTime_);

            // KernelTime includes IdleTime on Windows, so we need to exclude it for total
            ULONGLONG total = kernel + user; 
            
            // Calcul
            if (total > 0) {
                 cpuLoad_ = (double)(total - idle) * 100.0 / total;
            }

            // Save for next
            idleTime_ = newIdle;
            kernelTime_ = newKernel;
            userTime_ = newUser;
            return true;
        }

        MetricsMap getMetrics() const override {
            return { {"cpu_usage_percent", cpuLoad_} };
        }

    private:
        FILETIME idleTime_, kernelTime_, userTime_;
        double cpuLoad_ = 0.0;

        ULONGLONG u64(const FILETIME& ft) {
            return ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        }
    };

    class RamMonitor : public IMonitor {
    public:
        std::string getName() const override { return "SystemMonitorRAM"; }

        bool initialize() override { return true; }

        bool collect() override {
            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);
            GlobalMemoryStatusEx(&memInfo);

            totalPhys_ = memInfo.ullTotalPhys;
            availPhys_ = memInfo.ullAvailPhys;
            usedPhys_ = totalPhys_ - availPhys_;
            loadPercent_ = memInfo.dwMemoryLoad;
            
            return true;
        }

        MetricsMap getMetrics() const override {
            return {
                {"ram_total_bytes", (long long)totalPhys_},
                {"ram_used_bytes", (long long)usedPhys_},
                {"ram_load_percent", (double)loadPercent_}
            };
        }

    private:
        DWORD loadPercent_ = 0;
        DWORDLONG totalPhys_ = 0;
        DWORDLONG availPhys_ = 0;
        DWORDLONG usedPhys_ = 0;
    };
}
