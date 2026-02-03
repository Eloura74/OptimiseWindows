#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <winsvc.h>
#include <memory>
#include "../core/Logger.hpp"

namespace lsaa {

    // Structure représentant les informations d'un service Windows
    struct ServiceInfo {
        std::string name;           // Nom du service (ex: wuauserv)
        std::string displayName;    // Nom affiché (ex: Windows Update)
        std::string status;         // État actuel (Running, Stopped, etc.)
        DWORD pid;                  // ID du processus
        DWORD stateCode;            // Code d'état brut (SERVICE_RUNNING, etc.)
        bool canStop;               // Si le service peut être arrêté
        bool canPause;              // Si le service peut être mis en pause
    };

    class ServiceManager {
    public:
        // Récupère la liste de tous les services installés sur le système
        static std::vector<ServiceInfo> getServices() {
            std::vector<ServiceInfo> services;
            SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
            
            if (!hSCManager) {
                LSAA_LOG_ERROR("ServiceManager: Impossible d'ouvrir le SCM (Service Control Manager). Erreur: " + std::to_string(GetLastError()));
                return services;
            }

            DWORD bytesNeeded = 0;
            DWORD servicesReturned = 0;
            DWORD resumeHandle = 0;
            
            // Premier appel pour déterminer la taille du buffer nécessaire
            EnumServicesStatusEx(
                hSCManager, 
                SC_ENUM_PROCESS_INFO,
                SERVICE_WIN32, // Services de type Win32 (Driver exclus)
                SERVICE_STATE_ALL, // Tous les états (actifs et inactifs)
                NULL, 
                0, 
                &bytesNeeded, 
                &servicesReturned, 
                &resumeHandle, 
                NULL
            );

            // On alloue le buffer
            std::vector<BYTE> buffer(bytesNeeded);
            LPENUM_SERVICE_STATUS_PROCESS pServices = (LPENUM_SERVICE_STATUS_PROCESS)buffer.data();

            // Second appel pour récupérer les données
            if (EnumServicesStatusEx(
                hSCManager, 
                SC_ENUM_PROCESS_INFO,
                SERVICE_WIN32,
                SERVICE_STATE_ALL,
                (LPBYTE)pServices, 
                bytesNeeded, 
                &bytesNeeded, 
                &servicesReturned, 
                &resumeHandle, 
                NULL
            )) {
                for (DWORD i = 0; i < servicesReturned; i++) {
                    ServiceInfo info;
                    info.name = pServices[i].lpServiceName;
                    info.displayName = pServices[i].lpDisplayName;
                    info.pid = pServices[i].ServiceStatusProcess.dwProcessId;
                    info.stateCode = pServices[i].ServiceStatusProcess.dwCurrentState;
                    info.status = stateToString(info.stateCode);
                    
                    // Vérification des capacités (on pourrait aller plus loin avec QueryServiceStatusEx pour les détails)
                    // Pour simplifier ici, on considère que si ça tourne, on peut potentiellement l'arrêter
                    info.canStop = (info.stateCode == SERVICE_RUNNING);
                    
                    services.push_back(info);
                }
            } else {
                LSAA_LOG_ERROR("ServiceManager: Echec de l'énumération des services. Erreur: " + std::to_string(GetLastError()));
            }

            CloseServiceHandle(hSCManager);
            return services;
        }

        // Tente de démarrer un service par son nom
        static bool startService(const std::string& serviceName) {
            bool success = false;
            SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS); // Besoin de droits admin
            if (!hSCManager) return false;

            SC_HANDLE hService = OpenServiceA(hSCManager, serviceName.c_str(), SERVICE_START);
            if (hService) {
                if (StartServiceA(hService, 0, NULL)) {
                    LSAA_LOG_INFO("ServiceManager: Service démarré avec succès -> " + serviceName);
                    success = true;
                } else {
                    LSAA_LOG_ERROR("ServiceManager: Impossible de démarrer le service " + serviceName + ". Erreur: " + std::to_string(GetLastError()));
                }
                CloseServiceHandle(hService);
            }
            CloseServiceHandle(hSCManager);
            return success;
        }

        // Tente d'arrêter un service par son nom
        static bool stopService(const std::string& serviceName) {
            bool success = false;
            SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
            if (!hSCManager) return false;

            SC_HANDLE hService = OpenServiceA(hSCManager, serviceName.c_str(), SERVICE_STOP);
            if (hService) {
                SERVICE_STATUS status;
                if (ControlService(hService, SERVICE_CONTROL_STOP, &status)) {
                    LSAA_LOG_INFO("ServiceManager: Service arrêté avec succès -> " + serviceName);
                    success = true;
                } else {
                    LSAA_LOG_ERROR("ServiceManager: Impossible d'arrêter le service " + serviceName + ". Erreur: " + std::to_string(GetLastError()));
                }
                CloseServiceHandle(hService);
            }
            CloseServiceHandle(hSCManager);
            return success;
        }

    private:
        // Convertit le code d'état Windows en chaîne lisible
        static std::string stateToString(DWORD state) {
            switch (state) {
                case SERVICE_STOPPED: return "Arrêté";
                case SERVICE_START_PENDING: return "Démarrage...";
                case SERVICE_STOP_PENDING: return "Arrêt...";
                case SERVICE_RUNNING: return "En cours";
                case SERVICE_CONTINUE_PENDING: return "Reprise...";
                case SERVICE_PAUSE_PENDING: return "Mise en pause...";
                case SERVICE_PAUSED: return "En pause";
                default: return "Inconnu";
            }
        }
    };
}
