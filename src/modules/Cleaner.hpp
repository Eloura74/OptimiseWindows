#include <vector>
#include <filesystem>
#include <windows.h>
#include <ShlObj.h> 
#include <iostream>
#include "../core/Logger.hpp"

namespace fs = std::filesystem;

namespace lsaa {

    struct CleanParams {
        bool sysTemp = true;
        bool chrome = false;
        bool edge = false;
        bool firefox = false;
        bool dns = false;
    };

    struct DeleteStats {
        size_t files = 0;
        long long size = 0;
    };

    class Cleaner {
    public:
         Cleaner() {
             // System Temp
             char path[MAX_PATH];
             GetTempPathA(MAX_PATH, path);
             sysTempPath_ = std::string(path);

             // Local App Data
             char localAppData[MAX_PATH];
             if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
                 std::string base = std::string(localAppData);
                 chromeCache_ = base + "\\Google\\Chrome\\User Data\\Default\\Cache\\Cache_Data";
                 edgeCache_ = base + "\\Microsoft\\Edge\\User Data\\Default\\Cache\\Cache_Data";
                 
                 // Firefox needs profile search
                 std::string ffProfiles = base + "\\Mozilla\\Firefox\\Profiles";
                 if (fs::exists(ffProfiles)) {
                     for (const auto& entry : fs::directory_iterator(ffProfiles)) {
                         if (entry.is_directory() && entry.path().string().find(".default") != std::string::npos) {
                             firefoxCache_ = entry.path().string() + "\\cache2\\entries";
                             break; // Take first default profile
                         }
                     }
                 }
             }
         }

         struct ScanResult {
             size_t fileCount;
             long long totalSize;
         };

         ScanResult scan(const CleanParams& params) {
             ScanResult result = {0, 0};
             
             if (params.sysTemp) scanDir(sysTempPath_, result);
             if (params.chrome) scanDir(chromeCache_, result);
             if (params.edge) scanDir(edgeCache_, result);
             if (params.firefox) scanDir(firefoxCache_, result);
             
             // DNS doesn't have "size" really, maybe just count 1 "action"
             if (params.dns) result.fileCount++; 

             return result;
         }

         void clean(const CleanParams& params) {
             int deleted = 0;
             int failed = 0;

             if (params.sysTemp) cleanDir(sysTempPath_, deleted, failed);
             if (params.chrome) cleanDir(chromeCache_, deleted, failed);
             if (params.edge) cleanDir(edgeCache_, deleted, failed);
             if (params.firefox) cleanDir(firefoxCache_, deleted, failed);

             if (params.dns) {
                 // Run ipconfig /flushdns hidden
                 // This is a bit "hacky" but standard for simple tools
                 ShellExecuteA(NULL, "open", "ipconfig", "/flushdns", NULL, SW_HIDE);
                 deleted++;
                 LSAA_LOG_INFO("Cleaner: DNS Cache Flushed.");
             }

             LSAA_LOG_INFO("Cleaner: Finished. Deleted " + std::to_string(deleted) + " items. Locked/Skip: " + std::to_string(failed));
         }

         std::string getTempPath() const { return sysTempPath_; }

    private:
        std::string sysTempPath_;
        std::string chromeCache_;
        std::string edgeCache_;
        std::string firefoxCache_;

        void scanDir(const std::string& path, ScanResult& result) {
            if (path.empty() || !fs::exists(path)) return;
            try {
                for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)) {
                    if (fs::is_regular_file(entry)) {
                        result.fileCount++;
                        result.totalSize += fs::file_size(entry);
                    }
                }
            } catch (...) {}
        }

        void cleanDir(const std::string& path, int& deleted, int& failed) {
            if (path.empty() || !fs::exists(path)) return;
             for (const auto& entry : fs::directory_iterator(path)) {
                 try {
                     fs::remove_all(entry);
                     deleted++;
                 } catch (...) {
                     failed++;
                 }
             }
        }
    };
}
