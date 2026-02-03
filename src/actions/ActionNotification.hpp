#pragma once
#include "../engine/Rule.hpp"
#include <string>
#include <vector>
#include <windows.h>

namespace lsaa {

    class ActionNotification : public IAction {
    public:
        ActionNotification(std::string title, std::string message) 
            : title_(std::move(title)), message_(std::move(message)) {}

        void execute() override {
            // PowerScript command to show a balloon tip
            // Note: We use ExtractAssociatedIcon to get the current exe icon
            std::string psCommand = "powershell -WindowStyle Hidden -Command \"& {";
            psCommand += "Add-Type -AssemblyName System.Windows.Forms; ";
            psCommand += "Add-Type -AssemblyName System.Drawing; ";
            psCommand += "$notify = New-Object System.Windows.Forms.NotifyIcon; ";
            psCommand += "$notify.Icon = [System.Drawing.Icon]::ExtractAssociatedIcon((Get-Process -Id $pid).Path); ";
            psCommand += "$notify.Visible = $True; ";
            psCommand += "$notify.ShowBalloonTip(0, '" + title_ + "', '" + message_ + "', [System.Windows.Forms.ToolTipIcon]::Warning); ";
            // Need to sleep briefly to let the notification appear before disposal, but doing it in a detached process/thread is better 
            // simplifcation: we start it via CreateProcess so it runs async
            psCommand += "Start-Sleep -s 5; "; 
            psCommand += "$notify.Dispose()";
            psCommand += "}\"";

            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE; // Try to hide console
            ZeroMemory(&pi, sizeof(pi));

            std::vector<char> cmd(psCommand.begin(), psCommand.end());
            cmd.push_back(0);

            if (CreateProcessA(NULL, cmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                LSAA_LOG_INFO("Notification Sent: " + title_);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            } else {
                LSAA_LOG_ERROR("Failed to send notification.");
            }
        }

        std::string getName() const override { return "ActionNotification: " + title_; }

    private:
        std::string title_;
        std::string message_;
    };

}
