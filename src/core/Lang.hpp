#pragma once
#include <string>
#include <map>

namespace lsaa {

    enum class Language { EN, FR };

    class Lang {
    public:
        static Lang& instance() {
            static Lang instance;
            return instance;
        }

        void setLanguage(Language lang) {
            currentLang_ = lang;
        }

        Language getLanguage() const { return currentLang_; }
        
        void toggle() {
            currentLang_ = (currentLang_ == Language::EN) ? Language::FR : Language::EN;
        }

        const char* get(const std::string& key) {
            const auto& dict = (currentLang_ == Language::FR) ? fr_ : en_;
            if (dict.count(key)) return dict.at(key).c_str();
            return key.c_str();
        }

    private:
        Lang() {
            // --- ENGLISH ---
            en_["DASHBOARD"] = " DASHBOARD ";
            en_["RULES"] = " AUTOMATION RULES ";
            en_["OPTIMIZER"] = " CLEANER ";
            en_["STARTUP"] = " STARTUP APPS ";
            en_["SYSTEM_ONLINE"] = " | SYSTEM ONLINE";
            en_["PROCESSOR_LOAD"] = "PROCESSOR LOAD (CPU)";
            en_["MEMORY_USAGE"] = "MEMORY USAGE (RAM)";
            en_["QUICK_ACTIONS"] = "QUICK ACTIONS";
            en_["RUN_CLEANER"] = "RUN CLEAN SCRIPT";
            en_["TEST_NOTIF"] = "TEST NOTIFICATION";
            en_["TOP_PROCESSES"] = "TOP MEMORY CONSUMERS";
            en_["TopProcessDesc"] = "List of apps using the most memory. Click KILL to stop them forcefully.";
            en_["LOGS"] = "SYSTEM EVENT LOG";
            en_["LogsDesc"] = "Real-time log of system events and rule triggers.";
            en_["SAVE_CONFIG"] = "SAVE & APPLY CONFIGURATION";
            en_["SCAN_NOW"] = "SCAN FOR JUNK FILES";
            en_["CLEAN_ALL"] = "DELETE ALL JUNK FILES";
            en_["NO_JUNK"] = "Your system is clean! No temporary files found.";
            en_["FOUND_FILES"] = "Found %zu files taking up %.2f MB of space.";
            en_["STARTUP_MANAGER"] = "Startup Programs Manager";
            en_["ADD"] = "ADD PROGRAM";
            en_["REMOVE"] = "DISABLE";
            en_["PATH"] = "File Path";
            en_["NAME"] = "Application Name";
            en_["ACTION"] = "Action";
            
            // Explanations / Tooltips
            en_["DESC_DASHBOARD"] = "Overview of your computer's health. Monitor CPU/RAM usage and control active processes.";
            en_["DESC_RULES"] = "Automate your PC! Set limits (e.g., 'If RAM > 90%') and let the agent alert you or act automatically.";
            en_["DESC_OPTIMIZER"] = "Reclaim disk space by deleting temporary files and system leftovers safely.";
            en_["DESC_STARTUP"] = "Speed up your Windows boot time by disabling programs that start automatically.";
            en_["HELP_CPU"] = "Percentage of your processor's power currently being used.";
            en_["HELP_RAM"] = "Amount of short-term memory (RAM) used by open apps.";
            en_["HELP_KILL"] = "Forcefully crashes/stops the application. Data may be lost.";

            // --- FRANCAIS ---
            fr_["DASHBOARD"] = " TABLEAU DE BORD ";
            fr_["RULES"] = " REGLES D'AUTOMATISATION ";
            fr_["OPTIMIZER"] = " NETTOYEUR ";
            fr_["STARTUP"] = " DEMARRAGE ";
            fr_["SYSTEM_ONLINE"] = " | SYSTEME EN LIGNE";
            fr_["PROCESSOR_LOAD"] = "CHARGE DU PROCESSEUR (CPU)";
            fr_["MEMORY_USAGE"] = "MEMOIRE VIVE (RAM)";
            fr_["QUICK_ACTIONS"] = "ACTIONS RAPIDES";
            fr_["RUN_CLEANER"] = "LANCER LE NETTOYAGE";
            fr_["TEST_NOTIF"] = "TESTER LES NOTIFICATIONS";
            fr_["TOP_PROCESSES"] = "APPLICATIONS GOURMANDES";
            fr_["TopProcessDesc"] = "Liste des applications utilisant le plus de memoire. Cliquez sur TUER pour les arreter.";
            fr_["LOGS"] = "JOURNAL D'ACTIVITE";
            fr_["LogsDesc"] = "Historique en temps reel des actions du systeme.";
            fr_["SAVE_CONFIG"] = "SAUVEGARDER & APPLIQUER";
            fr_["SCAN_NOW"] = "ANALYSER LES FICHIERS INUTILES";
            fr_["CLEAN_ALL"] = "TOUT NETTOYER";
            fr_["NO_JUNK"] = "Votre systeme est propre ! Aucun fichier temporaire trouve.";
            fr_["FOUND_FILES"] = "%zu fichiers trouves occupant %.2f Mo d'espace.";
            fr_["STARTUP_MANAGER"] = "Gestionnaire de Demarrage Windows";
            fr_["ADD"] = "AJOUTER";
            fr_["REMOVE"] = "DESACTIVER";
            fr_["PATH"] = "Chemin du fichier";
            fr_["NAME"] = "Nom de l'application";
            fr_["ACTION"] = "Action";

            // Explications / Tooltips
            fr_["DESC_DASHBOARD"] = "Vue d'ensemble de la sante de votre PC. Surveillez le processeur, la memoire et les logiciels actifs.";
            fr_["DESC_RULES"] = "Automatisez votre PC ! Definissez des limites (ex: 'Si RAM > 90%') pour recevoir une alerte.";
            fr_["DESC_OPTIMIZER"] = "Liberez de l'espace disque en supprimant les fichiers temporaires et les residus systeme.";
            fr_["DESC_STARTUP"] = "Accelerez le demarrage de Windows en desactivant les logiciels qui se lancent inutilement.";
            fr_["HELP_CPU"] = "Pourcentage de la puissance de calcul actuellement utilisee.";
            fr_["HELP_RAM"] = "Quantite de memoire rapide (RAM) utilisee par vos applications ouvertes.";
            fr_["HELP_KILL"] = "Force l'arret immediat du logiciel. Des donnees non sauvegardees peuvent etre perdues.";
        }

        Language currentLang_ = Language::EN;
        std::map<std::string, std::string> en_;
        std::map<std::string, std::string> fr_;
    };
}
