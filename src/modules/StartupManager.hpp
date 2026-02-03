#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include "../core/Logger.hpp"

namespace lsaa {

    struct StartupItem {
        std::string name;
        std::string path;
    };

    class StartupManager {
    public:
        static std::vector<StartupItem> getStartupItems() {
            std::vector<StartupItem> items;
            HKEY hKey;
            if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                char valueName[16383];
                char data[16383];
                DWORD i = 0;
                DWORD valueNameLen = 16383;
                DWORD dataLen = 16383;
                DWORD type;

                while (RegEnumValueA(hKey, i++, valueName, &valueNameLen, NULL, &type, (LPBYTE)data, &dataLen) == ERROR_SUCCESS) {
                    items.push_back({std::string(valueName), std::string(data)});
                    valueNameLen = 16383;
                    dataLen = 16383;
                }
                RegCloseKey(hKey);
            }
            return items;
        }

        static bool addItem(const std::string& name, const std::string& path) {
            HKEY hKey;
            if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                if (RegSetValueExA(hKey, name.c_str(), 0, REG_SZ, (const BYTE*)path.c_str(), (DWORD)(path.size() + 1)) == ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    LSAA_LOG_INFO("Startup Item Added: " + name);
                    return true;
                }
                RegCloseKey(hKey);
            }
            return false;
        }

        static bool removeItem(const std::string& name) {
            HKEY hKey;
            if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                if (RegDeleteValueA(hKey, name.c_str()) == ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    LSAA_LOG_INFO("Startup Item Removed: " + name);
                    return true;
                }
                RegCloseKey(hKey);
            }
            return false;
        }
    };
}
