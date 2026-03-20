/**
 * @file securite.c
 * @brief Module 3 : Détection d'anomalies et analyse de sécurité
 *
 *  - DFS/BFS                : O(V + E)
 *  - Détection de cycles    : O(V + E)  DFS avec couleurs
 *  - Points d'articulation  : O(V + E)  DFS + low-link values
 *  - Ponts                  : O(V + E)  DFS + low-link values
 *  - Composantes connexes   : O(V + E)  DFS/BFS
 *  - Tarjan SCC             : O(V + E)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "securite.h"

/* ═══════════════════════════════════════════════════════════════════════
   FILE SIMPLE pour BFS
   ═══════════════════════════════════════════════════════════════════════ */
typedef struct { int data[MAX_NOEUDS]; int tete, queue; } FileBFS;
static void  fbfs_init(FileBFS* f) { f->tete = f->queue = 0; }
static int   fbfs_vide(FileBFS* f) { return f->tete == f->queue; }
static void  fbfs_push(FileBFS* f, int v) { f->data[f->queue++ % MAX_NOEUDS] = v; }
static int   fbfs_pop(FileBFS* f)  { return f->data[f->tete++ % MAX_NOEUDS]; }

/* PILE SIMPLE pour DFS itératif */
typedef struct { int data[MAX_NOEUDS]; int sommet; } PileDFS;
static void pile_init(PileDFS* p) { p->sommet = -1; }
static int  pile_vide(PileDFS* p) { return p->sommet < 0; }
static void pile_push(PileDFS* p, int v) { p->data[++p->sommet] = v; }
static int  pile_pop(PileDFS* p)  { return p->data[p->sommet--]; }

/* ═══════════════════════════════════════════════════════════════════════
   DFS — Parcours en profondeur (itératif)
   Complexité : O(V + E)
   ═══════════════════════════════════════════════════════════════════════ */
void dfs(Graphe* g, int depart) {
    if (!g || !noeud_existe(g, depart)) return;

    int visite[MAX_NOEUDS] = {0};
    PileDFS pile;
    pile_init(&pile);
    pile_push(&pile, depart);

    printf("\n  DFS depuis [%s] : ", g->noeuds[depart].nom);
    while (!pile_vide(&pile)) {
        int u = pile_pop(&pile);
        if (visite[u]) continue;
        visite[u] = 1;
        printf("%s ", g->noeuds[u].nom);

        /* Empiler les voisins (en ordre inverse pour respecter l'ordre gauche→droite) */
        Arete* voisins[MAX_NOEUDS];
        int nb = 0;
        Arete* a = g->noeuds[u].aretes;
        while (a) { voisins[nb++] = a; a = a->suivant; }
        for (int i = nb - 1; i >= 0; i--)
            if (!visite[voisins[i]->destination])
                pile_push(&pile, voisins[i]->destination);
    }
    printf("\n");
}

/* ═══════════════════════════════════════════════════════════════════════
   BFS — Parcours en largeur
   Complexité : O(V + E)
   ═══════════════════════════════════════════════════════════════════════ */
void bfs(Graphe* g, int depart) {
    if (!g || !noeud_existe(g, depart)) return;

    int     visite[MAX_NOEUDS] = {0};
    int     niveau[MAX_NOEUDS];
    FileBFS file;
    fbfs_init(&file);
    memset(niveau, -1, sizeof(niveau));

    visite[depart] = 1;
    niveau[depart] = 0;
    fbfs_push(&file, depart);

    printf("\n  BFS depuis [%s] :\n", g->noeuds[depart].nom);
    int niv_actuel = -1;
    while (!fbfs_vide(&file)) {
        int u = fbfs_pop(&file);
        if (niveau[u] != niv_actuel) {
            niv_actuel = niveau[u];
            printf("  Niveau %d : ", niv_actuel);
        }
        printf("%s ", g->noeuds[u].nom);

        Arete* a = g->noeuds[u].aretes;
        while (a) {
            if (!visite[a->destination]) {
                visite[a->destination]   = 1;
                niveau[a->destination]   = niv_actuel + 1;
                fbfs_push(&file, a->destination);
            }
            a = a->suivant;
        }
    }
    printf("\n");
}

/* ═══════════════════════════════════════════════════════════════════════
   DÉTECTION DE CYCLES — DFS avec 3 couleurs
   0=blanc(non vu), 1=gris(en cours), 2=noir(terminé)
   Complexité : O(V + E)
   ═══════════════════════════════════════════════════════════════════════ */
static int cycle_dfs_rec(Graphe* g, int u, int* couleur) {
    couleur[u] = 1; /* gris : en cours de visite */
    Arete* a = g->noeuds[u].aretes;
    while (a) {
        int v = a->destination;
        if (couleur[v] == 1) return 1;       /* arête vers nœud gris = cycle */
        if (couleur[v] == 0)
            if (cycle_dfs_rec(g, v, couleur)) return 1;
        a = a->suivant;
    }
    couleur[u] = 2; /* noir : terminé */
    return 0;
}

/**
 * @brief Détecte la présence de cycles dans le graphe
 * @return 1 si cycle(s) détecté(s), 0 sinon
 */
int detecter_cycles(Graphe* g) {
    if (!g) return 0;
    int couleur[MAX_NOEUDS] = {0};
    for (int i = 0; i < g->nb_noeuds; i++)
        if (couleur[i] == 0)
            if (cycle_dfs_rec(g, i, couleur)) return 1;
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
   POINTS D'ARTICULATION + PONTS
   Algorithme de Tarjan (version points d'articulation)
   Complexité : O(V + E)
   ═══════════════════════════════════════════════════════════════════════ */
static int timer_global;

static void ap_dfs(Graphe* g, int u, int parent,
                   int* visite, int* disc, int* low, int* ap,
                   int* ponts_src, int* ponts_dest, int* nb_ponts) {
    int enfants = 0;
    visite[u] = 1;
    disc[u] = low[u] = timer_global++;

    Arete* a = g->noeuds[u].aretes;
    while (a) {
        int v = a->destination;
        if (!visite[v]) {
            enfants++;
            ap_dfs(g, v, u, visite, disc, low, ap, ponts_src, ponts_dest, nb_ponts);
            if (low[v] < low[u]) low[u] = low[v];

            /* Point d'articulation */
            if ((parent == -1 && enfants > 1) ||
                (parent != -1 && low[v] >= disc[u]))
                ap[u] = 1;

            /* Pont */
            if (low[v] > disc[u]) {
                ponts_src[*nb_ponts]  = u;
                ponts_dest[*nb_ponts] = v;
                (*nb_ponts)++;
            }
        } else if (v != parent) {
            if (disc[v] < low[u]) low[u] = disc[v];
        }
        a = a->suivant;
    }
}

void points_articulation(Graphe* g, ResultatSecurite* res) {
    if (!g || !res) return;
    int visite[MAX_NOEUDS] = {0};
    int disc[MAX_NOEUDS], low[MAX_NOEUDS], ap[MAX_NOEUDS] = {0};
    memset(disc, -1, sizeof(disc));
    memset(low,  -1, sizeof(low));

    timer_global    = 0;
    res->nb_points_articulation = 0;
    res->nb_ponts               = 0;

    for (int i = 0; i < g->nb_noeuds; i++)
        if (!visite[i])
            ap_dfs(g, i, -1, visite, disc, low, ap,
                   res->ponts_src, res->ponts_dest, &res->nb_ponts);

    for (int i = 0; i < g->nb_noeuds; i++)
        if (ap[i])
            res->points_articulation[res->nb_points_articulation++] = i;
}

void ponts(Graphe* g, ResultatSecurite* res) {
    points_articulation(g, res); /* ponts calculés en même temps */
}

/* ═══════════════════════════════════════════════════════════════════════
   COMPOSANTES CONNEXES
   Complexité : O(V + E)
   ═══════════════════════════════════════════════════════════════════════ */
void composantes_connexes(Graphe* g, ResultatSecurite* res) {
    if (!g || !res) return;
    int visite[MAX_NOEUDS] = {0};
    memset(res->composantes, -1, sizeof(res->composantes));
    res->nb_composantes = 0;

    for (int i = 0; i < g->nb_noeuds; i++) {
        if (!visite[i]) {
            /* BFS pour marquer la composante */
            FileBFS f; fbfs_init(&f);
            fbfs_push(&f, i);
            visite[i] = 1;
            while (!fbfs_vide(&f)) {
                int u = fbfs_pop(&f);
                res->composantes[u] = res->nb_composantes;
                Arete* a = g->noeuds[u].aretes;
                while (a) {
                    if (!visite[a->destination]) {
                        visite[a->destination] = 1;
                        fbfs_push(&f, a->destination);
                    }
                    a = a->suivant;
                }
            }
            res->nb_composantes++;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
   TARJAN — Composantes Fortement Connexes (SCC)
   Complexité : O(V + E)
   ═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    int disc[MAX_NOEUDS];
    int low[MAX_NOEUDS];
    int sur_pile[MAX_NOEUDS];
    int pile[MAX_NOEUDS];
    int sommet_pile;
    int timer;
    int nb_scc;
} CtxTarjan;

static void tarjan_dfs(Graphe* g, int u, CtxTarjan* ctx) {
    ctx->disc[u] = ctx->low[u] = ctx->timer++;
    ctx->pile[++ctx->sommet_pile] = u;
    ctx->sur_pile[u] = 1;

    Arete* a = g->noeuds[u].aretes;
    while (a) {
        int v = a->destination;
        if (ctx->disc[v] == -1) {
            tarjan_dfs(g, v, ctx);
            if (ctx->low[v] < ctx->low[u]) ctx->low[u] = ctx->low[v];
        } else if (ctx->sur_pile[v]) {
            if (ctx->disc[v] < ctx->low[u]) ctx->low[u] = ctx->disc[v];
        }
        a = a->suivant;
    }

    /* Si u est une racine de SCC */
    if (ctx->low[u] == ctx->disc[u]) {
        printf("  SCC #%d : { ", ++ctx->nb_scc);
        int v;
        do {
            v = ctx->pile[ctx->sommet_pile--];
            ctx->sur_pile[v] = 0;
            printf("%s ", g->noeuds[v].nom);
        } while (v != u);
        printf("}\n");
    }
}

void composantes_fortement_connexes(Graphe* g) {
    if (!g) return;
    CtxTarjan ctx;
    memset(ctx.disc,     -1, sizeof(ctx.disc));
    memset(ctx.low,       0, sizeof(ctx.low));
    memset(ctx.sur_pile,  0, sizeof(ctx.sur_pile));
    ctx.sommet_pile = -1;
    ctx.timer       = 0;
    ctx.nb_scc      = 0;

    printf("\n  ─ Composantes Fortement Connexes (Tarjan) ─\n");
    for (int i = 0; i < g->nb_noeuds; i++)
        if (ctx.disc[i] == -1)
            tarjan_dfs(g, i, &ctx);
    printf("  Total : %d SCC\n", ctx.nb_scc);
}

/* ═══════════════════════════════════════════════════════════════════════
   RAPPORT DE SÉCURITÉ COMPLET
   ═══════════════════════════════════════════════════════════════════════ */
void analyser_securite(Graphe* g) {
    if (!g) return;
    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║       RAPPORT D'ANALYSE DE SÉCURITÉ         ║\n");
    printf("╠══════════════════════════════════════════════╣\n");

    /* Cycles */
    int cy = detecter_cycles(g);
    printf("  Cycles détectés       : %s\n", cy ? "⚠️  OUI (risque de boucle de routage)" : "✅ NON");

    /* Points d'articulation et ponts */
    ResultatSecurite res;
    memset(&res, 0, sizeof(res));
    points_articulation(g, &res);

    printf("  Points d'articulation : %d nœud(s) critique(s)\n", res.nb_points_articulation);
    for (int i = 0; i < res.nb_points_articulation; i++)
        printf("    ⚠️  Nœud [%d] %s — suppression déconnecterait le réseau\n",
               res.points_articulation[i],
               g->noeuds[res.points_articulation[i]].nom);

    printf("  Ponts (arêtes crit.)  : %d\n", res.nb_ponts);
    for (int i = 0; i < res.nb_ponts; i++)
        printf("    ⚠️  Arête %s → %s est un pont\n",
               g->noeuds[res.ponts_src[i]].nom,
               g->noeuds[res.ponts_dest[i]].nom);

    /* Composantes connexes */
    composantes_connexes(g, &res);
    printf("  Composantes connexes  : %d\n", res.nb_composantes);
    if (res.nb_composantes > 1) {
        printf("  ⚠️  Réseau non-connexe ! Sous-réseaux isolés :\n");
        for (int c = 0; c < res.nb_composantes; c++) {
            printf("    Composante %d : ", c);
            for (int i = 0; i < g->nb_noeuds; i++)
                if (res.composantes[i] == c)
                    printf("%s ", g->noeuds[i].nom);
            printf("\n");
        }
    }

    /* Nœuds non-sécurisés */
    printf("  Nœuds peu sécurisés  : ");
    int trouve = 0;
    for (int i = 0; i < g->nb_noeuds; i++) {
        Arete* a = g->noeuds[i].aretes;
        while (a) {
            if (a->securite < 5) {
                printf("[%s→%s sec=%d] ", g->noeuds[i].nom,
                       g->noeuds[a->destination].nom, a->securite);
                trouve = 1;
            }
            a = a->suivant;
        }
    }
    if (!trouve) printf("✅ Aucun");
    printf("\n");

    /* SCC */
    composantes_fortement_connexes(g);

    printf("╚══════════════════════════════════════════════╝\n");
}
