/**
 * @file main.c
 * @brief NetFlow Optimizer & Security Analyzer
 *        Menu interactif en ligne de commande
 *
 * ALC2101 — Algorithmique et Complexité — UVCI 2025-2026
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "graphe.h"
#include "liste_chainee.h"
#include "dijkstra.h"
#include "securite.h"
#include "utils.h"
#include "api.h"

/* ═══════════════════════════════════════════════════════════════════════
   ÉTAT GLOBAL
   ═══════════════════════════════════════════════════════════════════════ */
static Graphe*      G   = NULL;  /* graphe courant          */
static FileAttente* FA  = NULL;  /* file de paquets         */
static int          PAQUET_ID = 1;

/* ─── Lecture sécurisée ──────────────────────────────────────────────── */
static int lire_int(const char* invite) {
    int v; char buf[64];
    printf("%s", invite);
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    if (sscanf(buf, "%d", &v) != 1) return -1;
    return v;
}
static float lire_float(const char* invite) {
    float v; char buf[64];
    printf("%s", invite);
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return -1.0f;
    if (sscanf(buf, "%f", &v) != 1) return -1.0f;
    return v;
}
static void lire_str(const char* invite, char* buf, int sz) {
    printf("%s", invite);
    fflush(stdout);
    if (fgets(buf, sz, stdin)) {
        /* Retirer le \n */
        int len = (int)strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    }
}

static Metrique choisir_metrique(void) {
    printf("  Métrique : [0] Latence  [1] Coût  [2] Sécurité → ");
    int m = lire_int("");
    if (m < 0 || m > 2) m = 0;
    return (Metrique)m;
}

static int verifier_graphe(void) {
    if (!G) {
        printf("  [!] Aucun graphe chargé. Utilisez le menu 1.\n");
        return 0;
    }
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════
   MENU 1 — GESTION DU GRAPHE
   ═══════════════════════════════════════════════════════════════════════ */
static void menu_graphe(void) {
    int choix;
    do {
        afficher_titre("MODULE 1 : Gestion du Réseau (Graphe)");
        printf("  [1] Créer un graphe vide\n");
        printf("  [2] Charger depuis un fichier\n");
        printf("  [3] Charger le réseau de démonstration\n");
        printf("  [4] Générer un graphe aléatoire\n");
        printf("  [5] Ajouter une arête\n");
        printf("  [6] Supprimer une arête\n");
        printf("  [7] Afficher le graphe\n");
        printf("  [8] Afficher la matrice d'adjacence\n");
        printf("  [9] Sauvegarder dans un fichier\n");
        printf("  [0] ← Retour\n");
        choix = lire_int("  Choix : ");

        switch (choix) {
        case 1: {
            int n = lire_int("  Nombre de nœuds : ");
            int o = lire_int("  Orienté ? [1=oui, 0=non] : ");
            if (G) liberer_graphe(G);
            G = creer_graphe(n, o);
            if (G) printf("  [OK] Graphe de %d nœuds créé.\n", n);
            break;
        }
        case 2: {
            char f[256];
            lire_str("  Chemin du fichier : ", f, sizeof(f));
            charger_graphe(&G, f);
            break;
        }
        case 3:
            charger_graphe(&G, "data/reseau_demo.txt");
            break;
        case 4: {
            int n = lire_int("  Nombre de nœuds : ");
            int e = lire_int("  Nombre d'arêtes : ");
            int o = lire_int("  Orienté ? [1=oui, 0=non] : ");
            if (G) liberer_graphe(G);
            G = graphe_aleatoire(n, e, o);
            printf("  [OK] Graphe aléatoire créé.\n");
            break;
        }
        case 5:
            if (!verifier_graphe()) break;
            {
                int src  = lire_int("  Source      : ");
                int dest = lire_int("  Destination : ");
                float lat  = lire_float("  Latence (ms)      : ");
                float bw   = lire_float("  Bande passante (Mbps) : ");
                float cout = lire_float("  Coût               : ");
                int   sec  = lire_int("  Sécurité (0-10)   : ");
                ajouter_arete(G, src, dest, lat, bw, cout, sec);
                printf("  [OK] Arête %d → %d ajoutée.\n", src, dest);
            }
            break;
        case 6:
            if (!verifier_graphe()) break;
            {
                int src  = lire_int("  Source      : ");
                int dest = lire_int("  Destination : ");
                supprimer_arete(G, src, dest);
            }
            break;
        case 7:
            afficher_graphe(G);
            break;
        case 8:
            if (!verifier_graphe()) break;
            printf("\n  Matrice d'adjacence (latences) :\n     ");
            for (int j = 0; j < G->nb_noeuds; j++) printf("%6d", j);
            printf("\n");
            for (int i = 0; i < G->nb_noeuds; i++) {
                printf("  %2d |", i);
                for (int j = 0; j < G->nb_noeuds; j++) {
                    if (G->matrice_adj[i][j] >= INFINI) printf("     ∞");
                    else printf(" %5.1f", G->matrice_adj[i][j]);
                }
                printf("\n");
            }
            break;
        case 9:
            if (!verifier_graphe()) break;
            {
                char f[256];
                lire_str("  Nom du fichier : ", f, sizeof(f));
                sauvegarder_graphe(G, f);
            }
            break;
        }
    } while (choix != 0);
}

/* ═══════════════════════════════════════════════════════════════════════
   MENU 2 — ALGORITHMES DE ROUTAGE
   ═══════════════════════════════════════════════════════════════════════ */
static void menu_routage(void) {
    int choix;
    do {
        afficher_titre("MODULE 2 : Algorithmes de Routage Optimal");
        printf("  [1] Dijkstra (plus court chemin)\n");
        printf("  [2] Bellman-Ford\n");
        printf("  [3] Comparer Dijkstra vs Bellman-Ford\n");
        printf("  [4] Chemin avec contraintes (backtracking)\n");
        printf("  [5] K plus courts chemins\n");
        printf("  [0] ← Retour\n");
        choix = lire_int("  Choix : ");

        if (!verifier_graphe() && choix != 0) continue;

        switch (choix) {
        case 1: {
            int src  = lire_int("  Source      : ");
            int dest = lire_int("  Destination : ");
            Metrique m = choisir_metrique();
            printf("\n  ── Dijkstra ──────────────────────────────\n");
            double t0 = chrono_debut();
            Chemin* ch = dijkstra(G, src, dest, m);
            double t1 = chrono_fin(t0);
            afficher_chemin(ch, G);
            printf("  Temps d'exécution : %.4f ms\n", t1);
            free(ch);
            break;
        }
        case 2: {
            int src  = lire_int("  Source      : ");
            int dest = lire_int("  Destination : ");
            Metrique m = choisir_metrique();
            printf("\n  ── Bellman-Ford ──────────────────────────\n");
            double t0 = chrono_debut();
            Chemin* ch = bellman_ford(G, src, dest, m);
            double t1 = chrono_fin(t0);
            afficher_chemin(ch, G);
            printf("  Temps d'exécution : %.4f ms\n", t1);
            free(ch);
            break;
        }
        case 3: {
            int src  = lire_int("  Source      : ");
            int dest = lire_int("  Destination : ");
            Metrique m = choisir_metrique();
            printf("\n  ── Comparaison Dijkstra vs Bellman-Ford ──\n");

            double t0 = chrono_debut();
            Chemin* cd = dijkstra(G, src, dest, m);
            double td = chrono_fin(t0);

            double t1 = chrono_debut();
            Chemin* cb = bellman_ford(G, src, dest, m);
            double tb = chrono_fin(t1);

            printf("  DIJKSTRA    : coût=%.2f  temps=%.4f ms\n",
                   cd ? cd->cout_total : -1, td);
            printf("  BELLMAN-FORD: coût=%.2f  temps=%.4f ms\n",
                   cb ? cb->cout_total : -1, tb);
            printf("  Rapport BF/D : %.2fx\n", td > 0 ? tb/td : 0);
            free(cd); free(cb);
            break;
        }
        case 4: {
            int src  = lire_int("  Source          : ");
            int dest = lire_int("  Destination     : ");
            Contraintes c;
            memset(&c, 0, sizeof(c));
            c.bw_min   = lire_float("  BW min requise (Mbps) [0=aucune] : ");
            c.cout_max = lire_float("  Coût max autorisé  [9999=aucun] : ");
            c.sec_min  = lire_int("  Sécurité min (0-10) [0=aucune] : ");
            if (c.cout_max <= 0) c.cout_max = 999999.0f;

            printf("  Nœuds obligatoires (entrez -1 pour terminer) :\n");
            while (1) {
                int n = lire_int("    Nœud obligatoire : ");
                if (n < 0) break;
                c.noeuds_obligatoires[c.nb_obligatoires++] = n;
            }
            printf("  Nœuds exclus (entrez -1 pour terminer) :\n");
            while (1) {
                int n = lire_int("    Nœud exclu : ");
                if (n < 0) break;
                c.noeuds_exclus[c.nb_exclus++] = n;
            }
            Metrique m = choisir_metrique();

            printf("\n  ── Chemin contraint (backtracking) ───────\n");
            double t0 = chrono_debut();
            Chemin* ch = chemin_contraint(G, src, dest, &c, m);
            double t1 = chrono_fin(t0);
            afficher_chemin(ch, G);
            printf("  Temps d'exécution : %.4f ms\n", t1);
            free(ch);
            break;
        }
        case 5: {
            int src  = lire_int("  Source      : ");
            int dest = lire_int("  Destination : ");
            int k    = lire_int("  Valeur de K : ");
            Metrique m = choisir_metrique();
            k_plus_courts_chemins(G, src, dest, k, m);
            break;
        }
        }
    } while (choix != 0);
}

/* ═══════════════════════════════════════════════════════════════════════
   MENU 3 — SÉCURITÉ
   ═══════════════════════════════════════════════════════════════════════ */
static void menu_securite(void) {
    int choix;
    do {
        afficher_titre("MODULE 3 : Détection d'Anomalies et Sécurité");
        printf("  [1] Rapport de sécurité complet\n");
        printf("  [2] DFS depuis un nœud\n");
        printf("  [3] BFS depuis un nœud\n");
        printf("  [4] Détecter les cycles\n");
        printf("  [5] Points d'articulation (nœuds critiques)\n");
        printf("  [6] Ponts (arêtes critiques)\n");
        printf("  [7] Composantes connexes\n");
        printf("  [8] Composantes Fortement Connexes (Tarjan)\n");
        printf("  [0] ← Retour\n");
        choix = lire_int("  Choix : ");

        if (!verifier_graphe() && choix != 0) continue;

        switch (choix) {
        case 1: analyser_securite(G); break;
        case 2: { int n = lire_int("  Nœud de départ : "); dfs(G, n); break; }
        case 3: { int n = lire_int("  Nœud de départ : "); bfs(G, n); break; }
        case 4:
            printf("  Cycles : %s\n",
                   detecter_cycles(G) ? "⚠️  OUI détecté(s)" : "✅ Aucun cycle");
            break;
        case 5: {
            ResultatSecurite res; memset(&res, 0, sizeof(res));
            points_articulation(G, &res);
            printf("  Points d'articulation (%d) :\n", res.nb_points_articulation);
            for (int i = 0; i < res.nb_points_articulation; i++)
                printf("    ⚠️  [%d] %s\n", res.points_articulation[i],
                       G->noeuds[res.points_articulation[i]].nom);
            if (res.nb_points_articulation == 0) printf("    ✅ Aucun\n");
            break;
        }
        case 6: {
            ResultatSecurite res; memset(&res, 0, sizeof(res));
            ponts(G, &res);
            printf("  Ponts (%d) :\n", res.nb_ponts);
            for (int i = 0; i < res.nb_ponts; i++)
                printf("    ⚠️  %s → %s\n",
                       G->noeuds[res.ponts_src[i]].nom,
                       G->noeuds[res.ponts_dest[i]].nom);
            if (res.nb_ponts == 0) printf("    ✅ Aucun pont\n");
            break;
        }
        case 7: {
            ResultatSecurite res; memset(&res, 0, sizeof(res));
            composantes_connexes(G, &res);
            printf("  Composantes connexes : %d\n", res.nb_composantes);
            for (int c = 0; c < res.nb_composantes; c++) {
                printf("    C%d : ", c);
                for (int i = 0; i < G->nb_noeuds; i++)
                    if (res.composantes[i] == c) printf("%s ", G->noeuds[i].nom);
                printf("\n");
            }
            break;
        }
        case 8: composantes_fortement_connexes(G); break;
        }
    } while (choix != 0);
}

/* ═══════════════════════════════════════════════════════════════════════
   MENU 4 — FILE DE PAQUETS
   ═══════════════════════════════════════════════════════════════════════ */
static void menu_file(void) {
    int choix;
    do {
        afficher_titre("MODULE 4 : Gestion des Files de Paquets");
        printf("  [1] Créer/réinitialiser la file\n");
        printf("  [2] Enfiler un paquet (enqueue)\n");
        printf("  [3] Défiler un paquet (dequeue)\n");
        printf("  [4] Consulter la tête (peek)\n");
        printf("  [5] Afficher la file\n");
        printf("  [6] Simuler un flux de N paquets aléatoires\n");
        printf("  [7] Statistiques\n");
        printf("  [0] ← Retour\n");
        choix = lire_int("  Choix : ");

        switch (choix) {
        case 1: {
            if (FA) liberer_file(FA);
            int cap = lire_int("  Capacité max (0=illimitée) : ");
            FA = creer_file(cap);
            PAQUET_ID = 1;
            printf("  [OK] File créée (capacité %s).\n",
                   cap > 0 ? "limitée" : "illimitée");
            break;
        }
        case 2:
            if (!FA) { FA = creer_file(0); }
            if (!verifier_graphe()) {
                int src  = lire_int("  Source      : ");
                int dest = lire_int("  Destination : ");
                int prio = lire_int("  Priorité (1=haute, 10=basse) : ");
                float t  = lire_float("  Taille (Mo) : ");
                enqueue(FA, PAQUET_ID++, prio, t, src, dest, 0.0f);
            } else {
                int src  = lire_int("  Source      : ");
                int dest = lire_int("  Destination : ");
                int prio = lire_int("  Priorité (1=haute, 10=basse) : ");
                float t  = lire_float("  Taille (Mo) : ");
                enqueue(FA, PAQUET_ID++, prio, t, src, dest, 0.0f);
                printf("  [OK] Paquet P%d enfilé.\n", PAQUET_ID - 1);
            }
            break;
        case 3: {
            if (!FA) { printf("  [!] Pas de file.\n"); break; }
            Paquet* p = dequeue(FA);
            if (!p) { printf("  File vide.\n"); break; }
            printf("  [OUT] Paquet P%d | prio=%d | %.1f Mo | %d→%d\n",
                   p->id, p->priorite, p->taille_Mo, p->source, p->destination);
            free(p);
            break;
        }
        case 4: {
            if (!FA) { printf("  [!] Pas de file.\n"); break; }
            Paquet* p = peek(FA);
            if (!p) { printf("  File vide.\n"); break; }
            printf("  [PEEK] Paquet P%d | prio=%d | %.1f Mo | %d→%d\n",
                   p->id, p->priorite, p->taille_Mo, p->source, p->destination);
            break;
        }
        case 5:
            if (!FA) { printf("  [!] Pas de file.\n"); break; }
            afficher_file(FA);
            break;
        case 6: {
            if (!FA) FA = creer_file(20);
            int n = lire_int("  Nombre de paquets à simuler : ");
            printf("\n  Simulation de %d paquets...\n", n);
            srand(42);
            int nb_noeuds = G ? G->nb_noeuds : 5;
            for (int i = 0; i < n; i++) {
                int prio = 1 + rand() % 10;
                int src  = rand() % nb_noeuds;
                int dest = rand() % nb_noeuds;
                float t  = 0.1f + (float)(rand() % 100) / 10.0f;
                enqueue(FA, PAQUET_ID++, prio, t, src, dest, (float)i);
            }
            printf("\n  Traitement de la file :\n");
            while (!est_vide_file(FA)) {
                Paquet* p = dequeue(FA);
                printf("    [TRAITÉ] P%d prio=%d %d→%d\n",
                       p->id, p->priorite, p->source, p->destination);
                free(p);
            }
            afficher_stats_file(FA);
            break;
        }
        case 7:
            if (!FA) { printf("  [!] Pas de file.\n"); break; }
            afficher_stats_file(FA);
            break;
        }
    } while (choix != 0);
}

/* ═══════════════════════════════════════════════════════════════════════
   MENU 5 — ANALYSE DE PERFORMANCE
   ═══════════════════════════════════════════════════════════════════════ */
/*static void menu_performance(void) {
    afficher_titre("MODULE 5 : Analyse de Performance");
    printf("  Génération de graphes de tailles croissantes...\n\n");
    printf("  %-10s %-15s %-15s %-15s\n",
           "Nœuds", "Dijkstra(ms)", "BellmanFord(ms)", "Ratio BF/D");
    separateur();

    int tailles[] = {10, 25, 50, 100, 200};
    int nb = (int)(sizeof(tailles)/sizeof(tailles[0]));

    for (int t = 0; t < nb; t++) {
        int n = tailles[t];
        int e = n * 3;
        Graphe* g = graphe_aleatoire(n, e, 1);
        if (!g) continue;

        double td = 0, tb = 0;
        int essais = 5;
        for (int k = 0; k < essais; k++) {
            double t0 = chrono_debut();
            Chemin* cd = dijkstra(g, 0, n-1, METRIQUE_LATENCE);
            td += chrono_fin(t0);
            free(cd);

            t0 = chrono_debut();
            Chemin* cb = bellman_ford(g, 0, n-1, METRIQUE_LATENCE);
            tb += chrono_fin(t0);
            free(cb);
        }
        td /= essais; tb /= essais;
        printf("  %-10d %-15.4f %-15.4f %-15.2f\n",
               n, td, tb, td > 0 ? tb/td : 0);
        liberer_graphe(g);
    }
    separateur();
    printf("  Complexité théorique :\n");
    printf("    Dijkstra    : O((V+E) log V)\n");
    printf("    Bellman-Ford: O(V × E)\n");
    printf("  → BF est plus lent mais tolère les poids négatifs.\n");
}  */

static void menu_performance(void) {
    afficher_titre("MODULE 5 : Analyse de Performance");
    printf("  Génération de graphes de tailles croissantes...\n\n");
    printf("  %-6s %-8s %-16s %-16s %s\n",
           "Noeuds", "Aretes", "Dijkstra(ms)", "BellmanFord(ms)", "Ratio BF/D");
    separateur();

    /* Tailles, densité x10 pour que les temps soient mesurables */
    int tailles[] = { 20,    50,   100,   200,   500  };
    int aretes[]  = { 100,   500,  2000,  6000,  20000};
    int reps[]    = { 2000, 1000,   500,   200,    50  };
    int nb        = (int)(sizeof(tailles) / sizeof(tailles[0]));

    for (int t = 0; t < nb; t++) {
        int n    = tailles[t];
        int e    = aretes[t];
        int REPS = reps[t];

        srand(42 + t * 13);
        Graphe* g = graphe_aleatoire(n, e, 1);
        if (!g) continue;

        double td = 0.0, tb = 0.0;
        for (int r = 0; r < REPS; r++) {
            double t0;
            t0 = chrono_debut();
            Chemin* cd = dijkstra(g, 0, n - 1, METRIQUE_LATENCE);
            td += chrono_fin(t0);
            free(cd);

            t0 = chrono_debut();
            Chemin* cb = bellman_ford(g, 0, n - 1, METRIQUE_LATENCE);
            tb += chrono_fin(t0);
            free(cb);
        }
        liberer_graphe(g);

        double moy_d = td / REPS;
        double moy_b = tb / REPS;
        double ratio = moy_d > 1e-9 ? moy_b / moy_d : 0.0;

        printf("  %-6d %-8d %-16.4f %-16.4f %.2f\n",
               n, e, moy_d, moy_b, ratio);
    }

    separateur();
    printf("  Complexite theorique :\n");
    printf("    Dijkstra    : O((V+E) log V)\n");
    printf("    Bellman-Ford: O(V x E)\n");
    printf("  -> BF est plus lent mais tolere les poids negatifs.\n");
}

/* ═══════════════════════════════════════════════════════════════════════
   MENU PRINCIPAL
   ═══════════════════════════════════════════════════════════════════════ */
int main(int argc, char* argv[]) {

#ifdef _WIN32
    /* Force UTF-8 dans CMD, PowerShell et Git Bash — fix encodage ╔═╗ ✓ */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    /* Mode API JSON pour l'interface web */
    if (argc >= 2 && strcmp(argv[1], "--api") == 0) {
#ifdef _WIN32
        freopen("NUL", "w", stderr);
#else
        freopen("/dev/null", "w", stderr);
#endif
        return run_api_mode();
    }

    /* Mode interactif normal */
    /* Charger le réseau de démo au démarrage */
    charger_graphe(&G, "data/reseau_demo.txt");
    FA = creer_file(50);

    int choix;
    do {
        printf("\n");
        printf("  ╔══════════════════════════════════════════════════╗\n");
        printf("  ║   NetFlow Optimizer & Security Analyzer          ║\n");
        printf("  ║   ALC2101 — UVCI 2025-2026                       ║\n");
        printf("  ╠══════════════════════════════════════════════════╣\n");
        printf("  ║  [1] Module 1 — Gestion du Réseau (Graphe)       ║\n");
        printf("  ║  [2] Module 2 — Algorithmes de Routage           ║\n");
        printf("  ║  [3] Module 3 — Détection d'Anomalies/Sécurité   ║\n");
        printf("  ║  [4] Module 4 — File de Paquets                  ║\n");
        printf("  ║  [5] Module 5 — Analyse de Performance           ║\n");
        printf("  ║  [0] Quitter                                      ║\n");
        printf("  ╚══════════════════════════════════════════════════╝\n");
        if (G)
            printf("  Graphe actif : %d nœuds (%s)\n",
                   G->nb_noeuds, G->oriente ? "orienté" : "non-orienté");
        else
            printf("  Graphe actif : (aucun)\n");
        choix = lire_int("  Choix : ");

        switch (choix) {
        case 1: menu_graphe();      break;
        case 2: menu_routage();     break;
        case 3: menu_securite();    break;
        case 4: menu_file();        break;
        case 5: menu_performance(); break;
        case 0:
            printf("  Au revoir !\n");
            break;
        default:
            printf("  [!] Option invalide.\n");
        }
    } while (choix != 0);

    /* Nettoyage */
    if (G)  liberer_graphe(G);
    if (FA) liberer_file(FA);
    return 0;
}
