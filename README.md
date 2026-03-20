# NetFlow Optimizer & Security Analyzer

<div align="center">

![C](https://img.shields.io/badge/C-C11-blue?style=for-the-badge&logo=c)
![Python](https://img.shields.io/badge/Python-Flask-green?style=for-the-badge&logo=python)
![JavaScript](https://img.shields.io/badge/JavaScript-D3.js-yellow?style=for-the-badge&logo=javascript)
![License](https://img.shields.io/badge/License-MIT-purple?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-29%2F29%20✅-brightgreen?style=for-the-badge)

**Projet ALC2101 — Algorithmique et Complexité**  
UVCI · 2025-2026

[Démonstration](#démonstration) · [Installation](#installation) · [Architecture](#architecture) · [Modules](#modules) · [Tests](#tests)

</div>

---

## Présentation

**NetFlow Optimizer** est un système complet d'analyse et d'optimisation de réseaux informatiques, combinant des algorithmes de graphes implémentés en **C standard** avec une **interface web interactive** basée sur D3.js.

Le projet répond à une problématique réseau réelle : comment trouver le chemin optimal entre deux nœuds tout en respectant des contraintes de bande passante, de coût et de sécurité, et en détectant automatiquement les anomalies topologiques ?

### Fonctionnalités principales

- **Routage optimal** — Dijkstra, Bellman-Ford, K plus courts chemins, chemin contraint par backtracking
- **Analyse de sécurité** — Détection de cycles, points d'articulation, ponts, composantes fortement connexes (Tarjan)
- **File de paquets à priorité** — Implémentée avec liste chaînée, simulation de flux réseau
- **Benchmark comparatif** — Analyse empirique des complexités algorithmiques
- **Interface web interactive** — Visualisation D3.js, drag & drop, représentation adaptative (liste/matrice)

---

## Démonstration

### Interface Web

```
Réseau de 8 nœuds — Orienté
Dijkstra N0 → N7 : coût 45ms · chemin N0→N1→N2→N4→N5→N6→N7
Bellman-Ford     : coût 45ms · résultats identiques ✅
Rapport sécurité : 2 points critiques · 2 ponts · 11 liens non-sécurisés
```

### CLI interactif

```
  ╔══════════════════════════════════════════════════╗
  ║   NetFlow Optimizer & Security Analyzer          ║
  ║   ALC2101 — UVCI 2025-2026                       ║
  ╠══════════════════════════════════════════════════╣
  ║  [1] Module 1 — Gestion du Réseau (Graphe)       ║
  ║  [2] Module 2 — Algorithmes de Routage           ║
  ║  [3] Module 3 — Détection d'Anomalies/Sécurité   ║
  ║  [4] Module 4 — File de Paquets                  ║
  ║  [5] Module 5 — Analyse de Performance           ║
  ║  [0] Quitter                                      ║
  ╚══════════════════════════════════════════════════╝
```

---

## Architecture

```
NetFlowOptimizer/
├── src/
│   ├── main.c              # Point d'entrée — menu CLI + dispatch --api
│   ├── api.c / api.h       # Pont JSON stdin/stdout pour l'interface web
│   ├── graphe.c / .h       # Structure graphe, arêtes, chargement fichier
│   ├── dijkstra.c / .h     # Dijkstra, Bellman-Ford, K chemins, contraint
│   ├── securite.c / .h     # DFS, BFS, cycles, Tarjan, ponts, articulations
│   ├── liste_chainee.c / .h# File de priorité avec liste chaînée
│   └── utils.c / .h        # Chronomètre haute résolution, utilitaires
├── ui/
│   ├── netflow-optimizer.html  # Application web complète (D3.js)
│   └── style.css               # Design system dark/light
├── data/
│   ├── reseau_demo.txt     # Réseau de démonstration (8 nœuds, 14 arêtes)
│   └── reseau_etendu.txt   # Réseau étendu (13 nœuds, 26 arêtes)
├── tests/
│   └── tests_unitaires.c   # 29 tests unitaires (graphe, algos, file)
├── server.py               # Serveur Flask — passerelle HTTP ↔ binaire C
├── Makefile                # Compilation GCC cross-platform
└── start.bat               # Lanceur Windows tout-en-un
```

### Flux de données

```
Navigateur (D3.js)
    │  HTTP POST /api/routing/dijkstra
    ▼
server.py (Flask)
    │  subprocess: echo '{"cmd":"dijkstra","src":0,"dst":7}' | netflow.exe --api
    ▼
netflow.exe (C)
    │  Calcul algorithme → JSON stdout
    ▼
server.py → Navigateur (JSON response)
```

---

## Modules

### Module 1 — Gestion du Réseau

- Création/chargement de graphes depuis fichier `.txt`
- Ajout/suppression dynamique de nœuds et arêtes
- 4 métriques par arête : latence (ms), bande passante (Mbps), coût (€), sécurité (0-10)
- Représentation adaptative : **liste d'adjacence** si creux (densité < 20%), **matrice d'adjacence** si dense
- Sauvegarde de topologie

### Module 2 — Algorithmes de Routage Optimal

| Algorithme | Complexité | Cas d'usage |
|---|---|---|
| Dijkstra | O((V+E) log V) | Poids positifs, performance maximale |
| Bellman-Ford | O(V × E) | Poids négatifs possibles |
| K plus courts chemins | O(k(V+E) log V) | Redondance réseau |
| Chemin contraint | O(b^d) élagué | BW min + coût max + sécurité min |

### Module 3 — Détection d'Anomalies et Sécurité

- **DFS/BFS** — Parcours O(V+E) avec visualisation par niveaux
- **Détection de cycles** — DFS 3 couleurs O(V+E)
- **Points d'articulation** — Valeurs low-link de Tarjan O(V+E)
- **Ponts** — Arêtes critiques O(V+E)
- **Composantes connexes** — BFS multi-source O(V+E)
- **Tarjan SCC** — Composantes fortement connexes O(V+E)

### Module 4 — File de Paquets à Priorité

Implémentée avec **liste chaînée doublement chaînée** :

| Opération | Complexité |
|---|---|
| `enqueue` | O(n) — insertion triée par priorité |
| `dequeue` | O(1) — extraction de la tête |
| `peek` | O(1) — consultation sans suppression |

Simulation de flux avec statistiques : taux de perte, débit effectif, paquets perdus en cas de saturation.

### Module 5 — Analyse de Performance

Benchmark empirique sur graphes de tailles croissantes (V=20 à V=500) avec répétitions multiples pour mesures stables. Illustre la différence pratique entre O((V+E)logV) et O(V×E).

---

## Installation

### Prérequis

| Outil | Version | Rôle |
|---|---|---|
| GCC / MinGW-w64 | ≥ 11.0 | Compilation C |
| Python | ≥ 3.8 | Serveur Flask |
| Make | ≥ 4.0 | Automatisation build |

### Installation rapide — Windows

```bat
REM Double-cliquer sur start.bat
REM OU depuis CMD :
cmd /c start.bat
```

`start.bat` compile automatiquement le C, vérifie Python/Flask, et ouvre `http://localhost:5000`.

### Installation manuelle

```bash
# 1. Cloner le dépôt
git clone https://github.com/Adjoum/netflowOptimizeruvci-2026.git
cd netflowOptimizeruvci-2026

# 2. Installer les dépendances Python
pip install flask flask-cors

# 3. Compiler le projet C
make

# 4. Lancer l'interface web
make web
# → Ouvrir http://localhost:5000
```

### Mode CLI uniquement

```bash
make
./netflow.exe        # Windows
./netflow            # Linux/Mac
```

---

## Tests

```bash
make test
```

```
╔══════════════════════════════════════════════╗
║     TESTS UNITAIRES — NetFlow Optimizer      ║
╚══════════════════════════════════════════════╝
── Tests Graphe ──────────────────────────────
  [✓] Création graphe
  [✓] Nb nœuds correct
  [✓] Orienté
  [✓] Arête 0→1 créée
  [✓] Arête 0→1 latence
  [✓] Arête 1→0 absente (orienté)
  [✓] Matrice mise à jour
  [✓] Suppression arête 0→1
  [✓] Libération OK
  [✓] Non orienté : arête symétrique
── Tests File de Priorité ────────────────────
  [✓] Création file
  [✓] File vide init
  [✓] Taille après 3 enfilages
  [✓] Peek = priorité 1 (la plus haute)
  [✓] Dequeue : priorité 1 sort en premier
  [✓] Dequeue : priorité 3 sort ensuite
  [✓] File pleine a capacite 5
  [✓] Paquet perdu si file pleine
── Tests Dijkstra ────────────────────────────
  [✓] Dijkstra : chemin 0→3 trouvé
  [✓] Dijkstra : coût 0→3 = 15.0
  [✓] Dijkstra : chemin passe par 1
  [✓] Dijkstra : coût 0→4 = 25.0
  [✓] Dijkstra : pas de chemin 3→0 (orienté)
── Tests Bellman-Ford ────────────────────────
  [✓] Bellman-Ford : chemin 0→3
  [✓] Bellman-Ford : coût optimal 12.0
── Tests Sécurité ────────────────────────────
  [✓] Détection cycle (présent)
  [✓] Détection cycle (absent)
  [✓] Composantes connexes : 1
  [✓] Composantes connexes : 2 (non connexe)
══════════════════════════════════════════════
  Résultat : 29/29 tests réussis
  ✅ TOUS LES TESTS PASSÉS
══════════════════════════════════════════════
```

---

## Format du fichier réseau (.txt)

```
# nb_noeuds nb_aretes oriente(1=oui,0=non)
8 14 1
# src dst latence(ms) bande_passante(Mbps) cout securite(0-10)
0 1 10 1000 2 9
0 2 20 500 1.5 8
1 2 5 800 1 10
1 3 15 600 3 7
2 4 8 700 2 9
...
```

---

## Commandes Make disponibles

```bash
make            # Compiler le projet (→ netflow.exe)
make debug      # Compiler avec symboles de debug
make test       # Lancer les 29 tests unitaires
make clean      # Supprimer les binaires
make web        # Compiler + lancer Flask (http://localhost:5000)
make test-api   # Tester l'API JSON en ligne de commande
make install    # Installer Flask (pip install flask flask-cors)
```

---

## Test API en ligne de commande

```bash
# Dijkstra N0 → N7
echo '{"cmd":"dijkstra","src":0,"dst":7,"metric":"latence"}' | ./netflow.exe --api

# Détection de cycles
echo '{"cmd":"cycles"}' | ./netflow.exe --api

# Rapport de sécurité complet
echo '{"cmd":"security"}' | ./netflow.exe --api

# Benchmark
echo '{"cmd":"benchmark"}' | ./netflow.exe --api
```

---

## Technologies utilisées

| Couche | Technologie | Rôle |
|---|---|---|
| Algorithmes | C11 (GCC/MinGW) | Implémentation des algos de graphes |
| Serveur | Python 3 + Flask | Passerelle HTTP ↔ binaire C |
| Frontend | HTML5 + D3.js v7 | Visualisation interactive du graphe |
| Build | GNU Make | Automatisation compilation/tests |
| Lancement | Batch (.bat) | Déploiement Windows one-click |

---

## Auteurs:


-	**ADJOUMANI KOFFI WILFRID**
-	**HIEN BEBE** 
-	**KONAN KOUAME SERGE OLIVIER** 
-	**EFFI DOMINIQUE ALAIN ROMEO** 
-	**DAGO LAGBRE MOLIERE**
-	**KOUADIO KOFFI MODESTE**
-	**BROU YAPI ANGE ANDERSON**

Étudiants Master 1 Big Data Analytics — UVCI   
Côte d'Ivoire · 2025-2026

- Portfolio : [adjoumani-koffi.com](https://adjoumani-koffi.com)
- GitHub : [Adjoum](https://github.com/Adjoum)

---

## Licence

Ce projet est sous licence MIT — voir le fichier [LICENSE](LICENSE) pour les détails.

---

<div align="center">
<sub>ALC2101 — Algorithmique et Complexité · UVCI 2025-2026</sub>
</div>