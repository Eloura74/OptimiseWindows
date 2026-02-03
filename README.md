# LSAA - Local System Automation Agent

![LSAA Banner](https://via.placeholder.com/1200x300/0f1218/00bfff?text=LSAA+System+Automation+Agent)

**LSAA** est un agent d'automatisation système intelligent pour Windows, conçu pour surveiller, optimiser et nettoyer votre PC en temps réel. Il offre une interface moderne et élégante pour gérer les processus gourmands, automatiser des tâches via des règles, et nettoyer les fichiers inutiles.

> **Note**: Ce projet est en développement actif.

## 🚀 Fonctionnalités Clés

### 📊 Tableau de Bord (Dashboard)

- **Monitoring en Temps Réel** : Visualisez la charge CPU et l'utilisation RAM avec des graphiques historiques fluides.
- **Top Processus** : Identifiez instantanément les applications qui consomment le plus de mémoire.
- **Actions Rapides** : Tuez les processus bloqués ou lancez un nettoyage en un clic.

### 🤖 Moteur d'Automatisation (Rules Engine)

- **Règles "Si... Alors..."** : Créez des règles simples pour automatiser la gestion de votre PC.
- _Exemple_ : "Si la RAM dépasse 90%, envoyez-moi une notification."
- **Hot Reload** : Les règles sont appliquées immédiatement sans redémarrer l'application.

### 🧹 Nettoyeur Système (Optimizer)

- **Scan Intelligent** : Analyse les dossiers temporaires de Windows (`%TEMP%`).
- **Nettoyage Sécurisé** : Supprime uniquement les fichiers inutiles pour libérer de l'espace disque.

### 🚀 Gestionnaire de Démarrage (Startup Manager)

- **Accélérez le Boot** : Listez et désactivez les programmes qui se lancent au démarrage de Windows.
- **Contrôle Total** : Ajoutez vos propres programmes au démarrage ou supprimez les indésirables.

### 🌍 Support Multi-Langues

- Interface entièrement disponible en **Français** 🇫🇷 et **Anglais** 🇺🇸.
- Bascule instantanée entre les langues.

## 🛠️ Installation & Compilation

### Pré-requis

- **OS** : Windows 10/11 (64-bit)
- **Compilateur** : Visual Studio 2022 (MSVC) ou Build Tools
- **Outils** : CMake (3.20+), Git

### Compiler depuis les sources

1. Clonez le dépôt :

   ```cmd
   git clone https://github.com/votre-repo/LSAA.git
   cd OptimiserWin
   ```

2. Lancez le script de build automatique :

   ```cmd
   scripts\build_debug.bat
   ```

   _Ce script va configurer CMake, télécharger les dépendances (ImGui, GLFW, JSON) et compiler le projet._

3. Lancer l'application :
   ```cmd
   build\src\lsaa-core.exe
   ```

## 📸 Aperçu

| Dashboard                                                                                 | Automation Rules                                                                  |
| ----------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| ![Dashboard](https://via.placeholder.com/600x400/1a1a1a/cccccc?text=Dashboard+Screenshot) | ![Rules](https://via.placeholder.com/600x400/1a1a1a/cccccc?text=Rules+Screenshot) |

| Cleaner                                                                               | Startup Manager                                                                       |
| ------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| ![Cleaner](https://via.placeholder.com/600x400/1a1a1a/cccccc?text=Cleaner+Screenshot) | ![Startup](https://via.placeholder.com/600x400/1a1a1a/cccccc?text=Startup+Screenshot) |

## 🏗️ Architecture Technique

- **Langage** : C++20
- **GUI** : ImGui (Docking Branch) + GLFW + OpenGL3
- **Système** : Windows API (Win32, PDH, Registry)
- **Données** : JSON (nlohmann/json) pour la configuration
- **Architecture** :
  - `Core` : Engine, Logger, ConfigManager (Singleton)
  - `Modules` : Système de plugins pour les fonctionnalités (Cleaner, Startup)
  - `Monitors` : Collecte de données système (CPU, RAM, Process)

## � Licence

Ce projet est sous licence MIT. Libre à vous de le modifier et de le distribuer.

---

_Développé avec passion pour l'optimisation et l'automatisation._
