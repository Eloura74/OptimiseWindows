# LSAA - Local System Automation Agent

![LSAA Logo](https://via.placeholder.com/150?text=LSAA)

> **The Ultimate Windows Optimization & Automation Tool.**  
> _L'outil ultime d'optimisation et d'automatisation pour Windows._

---

## 🌍 Language / Langue

LSAA is fully bilingual (English / French). You can switch languages directly from the interface.  
_LSAA est entièrement bilingue (Anglais / Français). Vous pouvez changer la langue directement depuis l'interface._

---

## 🚀 Features / Fonctionnalités

### 1. Dashboard (Tableau de Bord)

- **Real-time Monitoring**: CPU Usage, RAM Usage, Top Memory Consumers.
- **Quick Actions**: One-click system cleaning and test tools.
- **Process Management**: Kill resource-heavy processes instantly.
- _Surveillance en temps réel : CPU, RAM, Processus gourmands._
- _Actions Rapides : Nettoyage système en un clic._

### 2. Smart Rules Engine (Moteur de Règles Intelligent)

- **Dynamic Configuration**: Create rules to automate actions based on system state.
- **Examples**:
  - "If CPU > 90%, send alert."
  - "If RAM > 85%, log warning."
- **Hot Reload**: Modify rules and apply them instantly without restarting the app.
- _Configuration Dynamique : Créez des règles pour automatiser les actions._
- _Rechargement à chaud : Modifiez les règles et appliquez-les instantanément._

### 3. System Optimizer (Nettoyeur)

- **Safe Cleaning**: Scans and removes temporary files (`%TEMP%`).
- **Disk Space Recovery**: Frees up valuable disk space safely.
- _Nettoyage Sûr : Scanne et supprime les fichiers temporaires._

### 4. Startup Manager (Gestionnaire de Démarrage)

- **Registry Control**: View programs starting with Windows (`HKCU\...\Run`).
- **Optimization**: Disable/Remove unnecessary startup programs to boost boot time.
- _Contrôle du Registre : Visualisez les programmes qui démarrent avec Windows._
- _Optimisation : Désactivez les programmes inutiles pour accélérer le démarrage._

---

## 🛠 Installation & Build

### Prerequisites / Prérequis

- Windows 10/11
- Visual Studio 2022 (C++ Desktop Development)
- CMake
- Git

### Building the Project / Compiler le Projet

```powershell
# 1. Clone the repository
git clone https://github.com/your-repo/lsaa.git
cd lsaa

# 2. Run the build script
scripts\build_debug.bat
```

The executable will be located in `build/src/lsaa-core.exe`.  
_L'exécutable se trouvera dans `build/src/lsaa-core.exe`._

---

## 💻 Technical Architecture / Architecture Technique

- **Language**: C++20
- **GUI**: ImGui + GLFW + OpenGL3
- **Build System**: CMake + NMake
- **Core Components**:
  - `Engine`: Main loop and monitor aggregation.
  - `RuleEngine`: logic for evaluating conditions (`ICondition`) and executing actions (`IAction`).
  - `ConfigManager`: JSON-based persistence (nlohmann/json).
  - `StartupManager`: Windows Registry Interaction API.

---

## 📸 Screenshots

_(Screenshots placeholders - to be updated with real images)_

---

**Author**: LSAA Team (Google Deepmind & User Collaboration)  
**License**: MIT
