# EXECUTION_PLAN : Plan d'Exécution LSAA

**Projet :** Local System Automation Agent (LSAA)
**Version :** 1.0.0
**Date :** 02/02/2026

Ce plan détaille les étapes de développement, de l'initialisation à la livraison finale.

## Phase 0 : Initialisation (Jour 0)

**Objectif :** Clean Project Base

- [ ] Création du dépôt Git.
- [ ] Structure des dossiers et `CMakeLists.txt` racine.
- [ ] Hello World C++20 compilant sous Windows (MSVC/Clang).
- [ ] Setup des outils d'analyse statique.

## Phase 1 : Core System (Semaine 1)

**Objectif :** Stable Non-Graphical Core

- **Modules :**
  - `SystemMonitorCPU` : Récupération % CPU via WinAPI (`GetSystemTimes`).
  - `SystemMonitorRAM` : Récupération RAM via `GlobalMemoryStatusEx`.
  - `ProcessMonitor` : Liste via `CreateToolhelp32Snapshot`.
  - `OberserverLoop` : Boucle principale avec `std::chrono`.
  - `Logger` : Logs console et fichier avec rotation.
- **Critères Done :**
  - [ ] L'application tourne 24h sans crash ni fuite.
  - [ ] CPU Idle < 1%.

## Phase 2 : Rule Engine (Semaine 2)

**Objectif :** Deterministic Decision Logic

- **Modules :**
  - Parser JSON pour les règles.
  - Moteur d'évaluation booléenne (`IF cpu > 90% THEN ...`).
  - Gestion de l'état (Stateful rules : "Si CPU > 90% pendant 5 min").
- **Critères Done :**
  - [ ] Evaluation < 5ms pour 100 règles.
  - [ ] Rechargement à chaud des règles.

## Phase 3 : Action Engine (Semaine 3)

**Objectif :** System Interaction

- **Actions :**
  - `KillProcess`, `SetPriority`.
  - `RunScript` (PowerShell, Batch).
  - `ShowNotification` (Toast Windows).
- **Critères Done :**
  - [ ] Les actions ne bloquent pas la boucle principale.
  - [ ] Gestion des erreurs d'exécution (timeout, permissions).

## Phase 4 : Graphical Interface (Semaine 4)

**Objectif :** Observabilité & Contrôle

- **Stack :** ImGui + Backend Win32/OpenGL (ou DX11).
- **Features :**
  - Fenêtre "Overlay" ou application classique.
  - Graphiques temps réel (Plot lines).
  - Liste des règles active/inactive.
- **Critères Done :**
  - [ ] 60 FPS stable.
  - [ ] Mode "Tray Icon" pour minimiser.

## Phase 5 : Extensions IA (Semaine 5 - Optionnel)

- Intégration API locale (Ollama / Llama.cpp server).
- Cas d'usage : "Analyse pourquoi mon PC est lent" (Raisonnement sur les logs).

## Phase 6 & 7 : Stabilisation & Release (Semaine 6-7)

- Création installateur ou package portable.
- Documentation utilisateur finale.
- Tests de sécurité (Permissions, Input fuzzing).
