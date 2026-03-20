#ifndef SECURITE_H
#define SECURITE_H

#include "graphe.h"

/* ─── Résultat d'analyse ─────────────────────────────────────────────── */
typedef struct {
    int points_articulation[MAX_NOEUDS];
    int nb_points_articulation;
    int ponts_src[MAX_NOEUDS];
    int ponts_dest[MAX_NOEUDS];
    int nb_ponts;
    int composantes[MAX_NOEUDS]; /* id de composante pour chaque nœud */
    int nb_composantes;
    int cycles_detectes;
} ResultatSecurite;

/* ─── API ────────────────────────────────────────────────────────────── */
/* Parcours */
void dfs(Graphe* g, int depart);
void bfs(Graphe* g, int depart);

/* Sécurité */
int  detecter_cycles(Graphe* g);
void points_articulation(Graphe* g, ResultatSecurite* res);
void ponts(Graphe* g, ResultatSecurite* res);
void composantes_connexes(Graphe* g, ResultatSecurite* res);
void composantes_fortement_connexes(Graphe* g); /* Tarjan */
void analyser_securite(Graphe* g);

#endif /* SECURITE_H */
