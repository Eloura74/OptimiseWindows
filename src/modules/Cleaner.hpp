#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <windows.h>
#include "../core/Logger.hpp"

namespace fs = std::filesystem;

namespace lsaa {

    class Cleaner {
    public:
         Cleaner() {
             // Get Temp Path
             char path[MAX_PATH];
             GetTempPathA(MAX_PATH, path);
             tempPath_ = std::string(path);
         }

         struct ScanResult {
             size_t fileCount;
             long long totalSize;
         };

         ScanResult scan() {
             ScanResult result = {0, 0};
             if (!fs::exists(tempPath_)) return result;

             try {
                 for (const auto& entry : fs::recursive_directory_iterator(tempPath_, fs::directory_options::skip_permission_denied)) {
                     if (fs::is_regular_file(entry)) {
                         result.fileCount++;
                         result.totalSize += fs::file_size(entry);
                     }
                 }
             } catch (...) {
                 // Ignore access errors during scan
             }
             return result;
         }

         void clean() {
             if (!fs::exists(tempPath_)) return;
             
             int deleted = 0;
             int failed = 0;

             for (const auto& entry : fs::directory_iterator(tempPath_)) {
                 try {
                     fs::remove_all(entry);
                     deleted++;
                 } catch (...) {
                     failed++;
                 }
             }
             LSAA_LOG_INFO("Cleaner: Deleted " + std::to_string(deleted) + " items. Skipped " + std::to_string(failed) + " (Locked).");
         }

         std::string getTempPath() const { return tempPath_; }

    private:
        std::string tempPath_;
    };
}
