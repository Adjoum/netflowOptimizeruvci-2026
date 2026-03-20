/**
 * @file graphe.c
 * @brief Implémentation du graphe pondéré (liste d'adjacence + matrice)
 * @complexity Création O(V²), ajout arête O(1), affichage O(V+E)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graphe.h"

/* ─── Création ───────────────────────────────────────────────────────── */
/**
 * @brief Alloue et initialise un graphe vide
 * @param nb_noeuds Nombre de nœuds
 * @param oriente   1=orienté, 0=non-orienté
 * @return Pointeur vers le graphe créé
 */
Graphe* creer_graphe(int nb_noeuds, int oriente) {
    if (nb_noeuds <= 0 || nb_noeuds > MAX_NOEUDS) {
        fprintf(stderr, "[ERREUR] Nombre de nœuds invalide : %d\n", nb_noeuds);
        return NULL;
    }

    Graphe* g = (Graphe*)malloc(sizeof(Graphe));
    if (!g) { perror("malloc Graphe"); return NULL; }

    g->nb_noeuds = nb_noeuds;
    g->oriente   = oriente;

    /* Tableau de nœuds */
    g->noeuds = (Noeud*)calloc(nb_noeuds, sizeof(Noeud));
    if (!g->noeuds) { perror("calloc noeuds"); free(g); return NULL; }

    for (int i = 0; i < nb_noeuds; i++) {
        g->noeuds[i].id     = i;
        g->noeuds[i].aretes = NULL;
        snprintf(g->noeuds[i].nom, NOM_MAX, "N%d", i);
    }

    /* Matrice d'adjacence : INFINI par défaut */
    g->matrice_adj = (float**)malloc(nb_noeuds * sizeof(float*));
    if (!g->matrice_adj) { perror("malloc matrice"); free(g->noeuds); free(g); return NULL; }
    for (int i = 0; i < nb_noeuds; i++) {
        g->matrice_adj[i] = (float*)malloc(nb_noeuds * sizeof(float));
        if (!g->matrice_adj[i]) { perror("malloc ligne matrice"); exit(EXIT_FAILURE); }
        for (int j = 0; j < nb_noeuds; j++)
            g->matrice_adj[i][j] = (i == j) ? 0.0f : INFINI;
    }
    return g;
}

/* ─── Ajout d'arête ──────────────────────────────────────────────────── */
/**
 * @brief Ajoute une arête orientée src→dest (et dest→src si non-orienté)
 */
void ajouter_arete(Graphe* g, int src, int dest,
                   float lat, float bw, float cout, int sec) {
    if (!g || src < 0 || dest < 0 || src >= g->nb_noeuds || dest >= g->nb_noeuds) {
        fprintf(stderr, "[ERREUR] Arête invalide : %d → %d\n", src, dest);
        return;
    }

    /* Création du nœud de liste chaînée */
    Arete* a = (Arete*)malloc(sizeof(Arete));
    if (!a) { perror("malloc Arete"); return; }
    a->destination    = dest;
    a->latence        = lat;
    a->bande_passante = bw;
    a->cout           = cout;
    a->securite       = (sec < 0) ? 0 : (sec > 10) ? 10 : sec;
    a->suivant        = g->noeuds[src].aretes; /* insertion en tête */
    g->noeuds[src].aretes = a;

    /* Mise à jour matrice (on stocke la latence) */
    g->matrice_adj[src][dest] = lat;

    /* Si non-orienté : arête symétrique */
    if (!g->oriente) {
        Arete* b = (Arete*)malloc(sizeof(Arete));
        if (!b) { perror("malloc Arete symétrique"); return; }
        *b = *a;
        b->destination = src;
        b->suivant     = g->noeuds[dest].aretes;
        g->noeuds[dest].aretes = b;
        g->matrice_adj[dest][src] = lat;
    }
}

/* ─── Suppression d'arête ────────────────────────────────────────────── */
void supprimer_arete(Graphe* g, int src, int dest) {
    if (!g) return;
    Arete* prev = NULL;
    Arete* cur  = g->noeuds[src].aretes;
    while (cur) {
        if (cur->destination == dest) {
            if (prev) prev->suivant   = cur->suivant;
            else      g->noeuds[src].aretes = cur->suivant;
            free(cur);
            g->matrice_adj[src][dest] = INFINI;
            printf("[OK] Arête %d → %d supprimée.\n", src, dest);
            return;
        }
        prev = cur;
        cur  = cur->suivant;
    }
    printf("[INFO] Arête %d → %d introuvable.\n", src, dest);
}

/* ─── Accès à une arête ──────────────────────────────────────────────── */
Arete* get_arete(Graphe* g, int src, int dest) {
    if (!g) return NULL;
    Arete* a = g->noeuds[src].aretes;
    while (a) {
        if (a->destination == dest) return a;
        a = a->suivant;
    }
    return NULL;
}

int noeud_existe(Graphe* g, int id) {
    return (g && id >= 0 && id < g->nb_noeuds);
}

/* ─── Affichage ──────────────────────────────────────────────────────── */
void afficher_graphe(Graphe* g) {
    if (!g) { printf("[INFO] Graphe NULL.\n"); return; }
    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║  GRAPHE (%s) — %d nœuds\n",
           g->oriente ? "orienté" : "non-orienté", g->nb_noeuds);
    printf("╠══════════════════════════════════════════════╣\n");
    printf("  %-6s %-10s  Arêtes [dest | lat | bw | coût | séc]\n",
           "ID", "Nom");
    printf("  ──────────────────────────────────────────────\n");
    for (int i = 0; i < g->nb_noeuds; i++) {
        printf("  [%3d] %-10s → ", i, g->noeuds[i].nom);
        Arete* a = g->noeuds[i].aretes;
        if (!a) { printf("(aucune)\n"); continue; }
        int first = 1;
        while (a) {
            if (!first) printf("                          ");
            printf("[%d | %.1fms | %.0fMbps | %.1f | %d/10]\n",
                   a->destination, a->latence,
                   a->bande_passante, a->cout, a->securite);
            a = a->suivant;
            first = 0;
        }
    }
    printf("╚══════════════════════════════════════════════╝\n");
}

/* ─── Chargement depuis fichier ──────────────────────────────────────── */
/**
 * Format fichier :
 *   nb_noeuds nb_aretes oriente(0/1)
 *   # commentaires ignorés
 *   src dest latence bande_passante cout securite
 */
void charger_graphe(Graphe** g, const char* fichier) {
    FILE* f = fopen(fichier, "r");
    if (!f) { perror(fichier); return; }

    int nb_n, nb_a, oriente;
    if (fscanf(f, "%d %d %d", &nb_n, &nb_a, &oriente) != 3) {
        fprintf(stderr, "[ERREUR] Format de fichier invalide.\n");
        fclose(f); return;
    }

    if (*g) liberer_graphe(*g);
    *g = creer_graphe(nb_n, oriente);
    if (!*g) { fclose(f); return; }

    char line[256];
    fgets(line, sizeof(line), f); /* consommer fin de ligne */
    int lus = 0;
    while (fgets(line, sizeof(line), f) && lus < nb_a) {
        if (line[0] == '#' || line[0] == '\n') continue;
        int src, dest, sec;
        float lat, bw, cout;
        if (sscanf(line, "%d %d %f %f %f %d",
                   &src, &dest, &lat, &bw, &cout, &sec) == 6) {
            ajouter_arete(*g, src, dest, lat, bw, cout, sec);
            lus++;
        }
    }
    fclose(f);
    printf("[OK] Graphe chargé : %d nœuds, %d arêtes depuis '%s'\n",
           nb_n, lus, fichier);
}

/* ─── Sauvegarde ─────────────────────────────────────────────────────── */
void sauvegarder_graphe(Graphe* g, const char* fichier) {
    if (!g) return;
    FILE* f = fopen(fichier, "w");
    if (!f) { perror(fichier); return; }

    /* Compter les arêtes */
    int nb_a = 0;
    for (int i = 0; i < g->nb_noeuds; i++) {
        Arete* a = g->noeuds[i].aretes;
        while (a) { nb_a++; a = a->suivant; }
    }

    fprintf(f, "%d %d %d\n", g->nb_noeuds, nb_a, g->oriente);
    fprintf(f, "# src dest latence bande_passante cout securite\n");
    for (int i = 0; i < g->nb_noeuds; i++) {
        Arete* a = g->noeuds[i].aretes;
        while (a) {
            fprintf(f, "%d %d %.2f %.2f %.2f %d\n",
                    i, a->destination, a->latence,
                    a->bande_passante, a->cout, a->securite);
            a = a->suivant;
        }
    }
    fclose(f);
    printf("[OK] Graphe sauvegardé dans '%s'\n", fichier);
}

/* ─── Libération mémoire ─────────────────────────────────────────────── */
void liberer_graphe(Graphe* g) {
    if (!g) return;
    for (int i = 0; i < g->nb_noeuds; i++) {
        Arete* a = g->noeuds[i].aretes;
        while (a) {
            Arete* tmp = a->suivant;
            free(a);
            a = tmp;
        }
    }
    for (int i = 0; i < g->nb_noeuds; i++)
        free(g->matrice_adj[i]);
    free(g->matrice_adj);
    free(g->noeuds);
    free(g);
}
