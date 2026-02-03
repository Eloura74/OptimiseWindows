# Reste à Faire - LSAA (Local System Automation Agent)

Ce document liste les fonctionnalités manquantes, les améliorations possibles et les dettes techniques restantes pour atteindre une version v1.0 stable et complète.

## 1. Fonctionnalités Principales Manquantes

### Services Windows

- [ ] **Gestionnaire de Services** : Lister les services (`services.msc`), afficher leur état (Running/Stopped) et permettre de les Démarrer/Arrêter/Désactiver.
- [ ] **Optimisation Automatique** : Suggérer la désactivation de services inutiles connus (Télémétrie, Fax, etc.).

### Nettoyage Avancé

- [ ] **Plus de cibles** : Vider le Cache DNS, les logs Windows Update, les fichiers temporaires des navigateurs (Chrome/Edge/Firefox).
- [ ] **Planification** : Exécuter le nettoyage automatiquement tous les X jours.

### Règles d'Automatisation (Engine)

- [ ] **Conditions Complexes** : Supporter `ET` / `OU` (ex: Si CPU > 90% ET RAM > 80%).
- [ ] **Durée** : "Si CPU > 90% pendant plus de 30 secondes".
- [ ] **Actions Supplémentaires** : Lancer un script PowerShell, Changer la priorité d'un processus, Redémarrer le PC.

## 2. Interface Utilisateur (GUI) & Expérience

- [ ] **Persistance des Fenêtres** : Sauvegarder la position et la taille de la fenêtre LSAA à la fermeture.
- [ ] **Thèmes Personnalisables** : Permettre à l'utilisateur de choisir une couleur d'accentuation autre que le bleu par défaut.
- [ ] **Tray Icon** : Réduire l'application dans la zone de notification (System Tray) au lieu de fermer, pour qu'elle continue de surveiller en arrière-plan.

## 3. Technique & Devops

- [ ] **Installateur** : Créer un installateur `.msi` ou `.exe` (Inno Setup / WiX) pour distribuer l'application facilement sans demander de compiler.
- [ ] **Tests Unitaires** : Ajouter des tests pour `SystemMonitor` et `ProcessMonitor` afin d'éviter les régressions de métriques (le bug du 0%).
- [ ] **Logs Rotatifs** : Éviter que le fichier de log ne grossisse indéfiniment.

## 4. Documentation

- [x] README.md (Fait)
- [x] Commentaires Code (Fait)
- [ ] **Wiki Utilisateur** : Un petit guide PDF ou Web expliquant aux novices comment créer une règle ou quoi nettoyer.

---

_Généré par Antigravity Agent - 2026_
