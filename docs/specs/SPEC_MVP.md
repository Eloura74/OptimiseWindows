# SPEC_MVP : Spécifications du Minimum Viable Product (Phase 0 & 1)

**Projet :** Local System Automation Agent (LSAA)
**Version :** 1.0.0
**Date :** 02/02/2026

## 1. Vision & Objectifs MVP

L'objectif du MVP est de délivrer le **Cœur du Système (Core System)** : une application console stable, ultra-performante et locale, capable de surveiller les métriques de base du système sans impacter ses performances.

### Objectifs Principaux

1.  **Fondations Solides :** Base de code C++20 propre, moderne et extensible.
2.  **Observabilité de Base :** Capacité à lire CPU, RAM et Processus.
3.  **Performance Critique :** < 1% CPU en idle, < 2s au démarrage.
4.  **Stabilité :** Fonctionnement 24h/24 sans fuite de mémoire.

## 2. Périmètre Fonctionnel (Scope)

### 2.1. Initialisation (Phase 0)

- **Structure du projet :** CMake, organisation des dossiers (src, include, tests, docs).
- **Outillage :** Scripts de build (Windows), configuration Git.
- **Livrable :** Un exécutable `lsaa-core.exe` qui se lance et s'arrête proprement.

### 2.2. Modules de Surveillance (Phase 1)

Le système doit interroger les API natives (WinAPI pour Windows) pour récupérer :

- **CPU Usage :** Utilisation globale en pourcentage.
- **RAM Usage :** Mémoire totale, utilisée, libre.
- **Process List :** Liste des processus actifs (PID, Nom, Utilisation Mémoire).

### 2.3. Moteur Principal (Core Loop)

- **Boucle Non-Bloquante :** Architecture basée sur une boucle d'événements ou un polling intelligent.
- **Fréquence :** Configurable (par défaut 1s ou 500ms).
- **Logging :** Système de logs structurés (Console + Fichier rotatif) pour le débogage et l'audit.

## 3. Contraintes Techniques Non-Négociables

### 3.1. Performance

- **CPU Overhead :** Doit rester strictement inférieur à **1%** en moyenne sur une machine "idle".
- **Mémoire :** Empreinte mémoire minimale (< 50 Mo visé pour le core).
- **Démarrage :** < 2 secondes.

### 3.2. Architecture & Code

- **Langage :** C++20 Standard.
- **Build System :** CMake.
- **OS Cible :** Windows 10/11 (Priorité 1), Architecture prête pour Linux.
- **Dépendances :** Aucune dépendance "lourde" (pas de framework web, pas de runtime complexe). Utilisation de bibliothèques C++ modernes (std, fmt si besoin, nlohmann/json).
- **Sécurité :** Pas de télémétrie, pas d'appels cloud, permissions locales uniquement.

## 4. Architecture Logique (Aperçu MVP)

```mermaid
graph TD
    Main[Main Entry Point] --> Core[Core Engine]
    Core --> Config[Config Loader]
    Core --> Logger[Structured Logger]
    Core --> MonitorMgr[Monitor Manager]

    MonitorMgr --> CpuMon[CPU Monitor (WinAPI)]
    MonitorMgr --> RamMon[RAM Monitor (WinAPI)]
    MonitorMgr --> ProcMon[Process Monitor (WinAPI)]

    Core -- Loop --> MonitorMgr
    MonitorMgr -- Data --> Core
    Core -- Log --> Logger
```

## 5. Critères de Validation (Definition of Done)

- [ ] La compilation fonctionne via CMake sans erreur ni warning majeur.
- [ ] L'exécutable se lance, affiche les métriques CPU/RAM dans la console et loggue dans un fichier.
- [ ] Test d'endurance : Le processus tourne 24h sans augmentation de la consommation mémoire (Fuite = 0).
- [ ] Test de performance : L'usage CPU du processus `lsaa-core` est < 1% en monitoring actif.
- [ ] Le code respecte les principes SOLID et est commenté en français.
