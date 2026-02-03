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
        
        // Helper to toggle
        void toggle() {
            if (currentLang_ == Language::EN) currentLang_ = Language::FR;
            else currentLang_ = Language::EN;
        }

        const char* get(const std::string& key) {
            if (currentLang_ == Language::FR) {
                if (fr_.count(key)) return fr_.at(key).c_str();
            } else {
                if (en_.count(key)) return en_.at(key).c_str();
            }
            return key.c_str(); // Fallback to key
        }

    private:
        Lang() {
            // ENGLISH DEF
            en_["DASHBOARD"] = " DASHBOARD ";
            en_["RULES"] = " RULES CONFIG ";
            en_["OPTIMIZER"] = " OPTIMIZER ";
            en_["STARTUP"] = " STARTUP ";
            en_["SYSTEM_ONLINE"] = "| SYSTEM ONLINE";
            en_["PROCESSOR_LOAD"] = "PROCESSOR LOAD";
            en_["MEMORY_USAGE"] = "MEMORY USAGE";
            en_["QUICK_ACTIONS"] = "QUICK ACTIONS";
            en_["RUN_CLEANER"] = "RUN CLEAN SCRIPT";
            en_["TEST_NOTIF"] = "TEST NOTIFICATION";
            en_["TOP_PROCESSES"] = "TOP MEMORY CONSUMERS";
            en_["LOGS"] = "SYSTEM EVENT LOG";
            en_["SAVE_CONFIG"] = "SAVE CONFIGURATION";
            en_["SCAN_NOW"] = "SCAN NOW";
            en_["CLEAN_ALL"] = "CLEAN ALL";
            en_["NO_JUNK"] = "No junk files found or scan not run.";
            en_["FOUND_FILES"] = "Found %zu files taking up %.2f MB";
            en_["STARTUP_MANAGER"] = "Startup Programs Manager (Registry: HKCU\\...\\Run)";
            en_["ADD"] = "ADD";
            en_["REMOVE"] = "REMOVE";
            en_["PATH"] = "Path";
            en_["NAME"] = "Name";
            en_["ACTION"] = "Action";

            // FRENCH DEF
            fr_["DASHBOARD"] = " TABLEAU DE BORD ";
            fr_["RULES"] = " CONFIGURATION REGLES ";
            fr_["OPTIMIZER"] = " OPTIMISEUR ";
            fr_["STARTUP"] = " DEMARRAGE ";
            fr_["SYSTEM_ONLINE"] = "| SYSTEME EN LIGNE";
            fr_["PROCESSOR_LOAD"] = "CHARGE PROCESSEUR";
            fr_["MEMORY_USAGE"] = "UTILISATION MEMOIRE";
            fr_["QUICK_ACTIONS"] = "ACTIONS RAPIDES";
            fr_["RUN_CLEANER"] = "LANCER NETTOYAGE SCRIPTS";
            fr_["TEST_NOTIF"] = "TESTER NOTIFICATION";
            fr_["TOP_PROCESSES"] = "TOP CONSOMMATION MEMOIRE";
            fr_["LOGS"] = "JOURNAL EVENEMENTS SYSTEME";
            fr_["SAVE_CONFIG"] = "SAUVEGARDER CONFIG";
            fr_["SCAN_NOW"] = "SCANNER MAINTENANT";
            fr_["CLEAN_ALL"] = "TOUT NETTOYER";
            fr_["NO_JUNK"] = "Aucun fichier inutile trouve ou scan non lance.";
            fr_["FOUND_FILES"] = "%zu fichiers trouves occupant %.2f MB";
            fr_["STARTUP_MANAGER"] = "Gestionnaire de Demarrage (Registre: HKCU\\...\\Run)";
            fr_["ADD"] = "AJOUTER";
            fr_["REMOVE"] = "SUPPRIMER";
            fr_["PATH"] = "Chemin";
            fr_["NAME"] = "Nom";
            fr_["ACTION"] = "Action";
        }

        Language currentLang_ = Language::EN;
        std::map<std::string, std::string> en_;
        std::map<std::string, std::string> fr_;
    };
}
