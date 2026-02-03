#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "../core/IMonitor.hpp"

namespace lsaa {

    class SystemMonitor : public IMonitor {
    public:
        std::string getName() const override { return "SystemMonitor"; }

        bool initialize() override {
            // Initial snapshot for CPU calculation
            GetSystemTimes((FILETIME*)&prevIdle_, (FILETIME*)&prevKernel_, (FILETIME*)&prevUser_);
            return true;
        }

        bool collect() override {
            // 1. Memory
            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);
            GlobalMemoryStatusEx(&memInfo);
            ramTotal_ = memInfo.ullTotalPhys;
            ramUsed_ = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
            ramLoad_ = memInfo.dwMemoryLoad;

            // 2. CPU
            FILETIME idle, kernel, user;
            if (GetSystemTimes(&idle, &kernel, &user)) {
                unsigned long long currentIdle = ftToUll(idle);
                unsigned long long currentKernel = ftToUll(kernel);
                unsigned long long currentUser = ftToUll(user);

                unsigned long long deltaIdle = currentIdle - prevIdle_;
                unsigned long long deltaKernel = currentKernel - prevKernel_;
                unsigned long long deltaUser = currentUser - prevUser_;

                unsigned long long totalSys = deltaKernel + deltaUser;
                // If total is 0, keep previous val
                if (totalSys > 0) {
                     // Kernel time includes Idle time on some systems? No, usually separate or Kernel includes idle. 
                     // Correct formula: (Total - Idle) / Total * 100
                     // But wait, GetSystemTimes docs: "KernelTime includes IdleTime".
                     // So Total = Kernel + User.  RealUsage = ( (Kernel - Idle) + User ) / (Kernel + User)
                     
                     unsigned long long active = (deltaKernel - deltaIdle) + deltaUser;
                     cpuLoad_ = (double)active * 100.0 / (double)totalSys;
                }

                prevIdle_ = currentIdle;
                prevKernel_ = currentKernel;
                prevUser_ = currentUser;
            }

            return true;
        }

        MetricsMap getMetrics() const override {
            return {
                {"cpu_usage_percent", cpuLoad_},
                {"ram_total_bytes", (long long)ramTotal_},
                {"ram_used_bytes", (long long)ramUsed_},
                {"ram_load_percent", (double)ramLoad_}
            };
        }

    private:
        unsigned long long prevIdle_ = 0;
        unsigned long long prevKernel_ = 0;
        unsigned long long prevUser_ = 0;

        double cpuLoad_ = 0.0;
        unsigned long long ramTotal_ = 0;
        unsigned long long ramUsed_ = 0;
        DWORD ramLoad_ = 0;

        unsigned long long ftToUll(const FILETIME& ft) {
            ULARGE_INTEGER uli;
            uli.LowPart = ft.dwLowDateTime;
            uli.HighPart = ft.dwHighDateTime;
            return uli.QuadPart;
        }
    };
}
