#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graphe.h"

/* ─── Résultat de chemin ─────────────────────────────────────────────── */
typedef struct {
    int   chemin[MAX_NOEUDS]; /* séquence des nœuds  */
    int   longueur;           /* nb de nœuds         */
    float cout_total;         /* coût selon métrique */
    float latence_totale;
    float bw_minimale;        /* goulot d'étranglement */
    int   securite_min;       /* maillon le plus faible */
} Chemin;

/* ─── Métrique de routage ─────────────────────────────────────────────  */
typedef enum {
    METRIQUE_LATENCE = 0,
    METRIQUE_COUT,
    METRIQUE_SECURITE  /* on minimise (10 - securite) */
} Metrique;

/* ─── Contraintes pour backtracking ─────────────────────────────────── */
typedef struct {
    float bw_min;          /* bande passante minimale requise  */
    float cout_max;        /* coût total maximal autorisé      */
    int   sec_min;         /* niveau de sécurité minimal       */
    int   noeuds_obligatoires[MAX_NOEUDS];
    int   nb_obligatoires;
    int   noeuds_exclus[MAX_NOEUDS];
    int   nb_exclus;
} Contraintes;

/* ─── API ────────────────────────────────────────────────────────────── */
/* Dijkstra */
Chemin* dijkstra(Graphe* g, int source, int dest, Metrique m);
/* Bellman-Ford (supporte poids négatifs) */
Chemin* bellman_ford(Graphe* g, int source, int dest, Metrique m);
/* K plus courts chemins */
void    k_plus_courts_chemins(Graphe* g, int src, int dest,
                               int k, Metrique m);
/* Chemin avec contraintes (backtracking) */
Chemin* chemin_contraint(Graphe* g, int src, int dest,
                          Contraintes* c, Metrique m);

/* Utilitaires */
void    afficher_chemin(Chemin* ch, Graphe* g);
float   poids_arete(Arete* a, Metrique m);

#endif /* DIJKSTRA_H */
