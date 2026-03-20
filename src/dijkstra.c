/**
 * @file dijkstra.c
 * @brief Module 2 : algorithmes de routage optimal
 *
 *  - Dijkstra         : O((V+E) log V)  glouton + file de priorité
 *  - Bellman-Ford     : O(V·E)          programmation dynamique
 *  - Backtracking     : O(b^d) élagué   chemins contraints
 *  - K plus courts    : diviser-régner  K meilleures alternatives
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "dijkstra.h"

/* ═══════════════════════════════════════════════════════════════════════
   Utilitaires
   ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Retourne le poids d'une arête selon la métrique choisie
 */
float poids_arete(Arete* a, Metrique m) {
    if (!a) return INFINI;
    switch (m) {
        case METRIQUE_LATENCE:  return a->latence;
        case METRIQUE_COUT:     return a->cout;
        case METRIQUE_SECURITE: return (float)(10 - a->securite); /* minimiser */
        default:                return a->latence;
    }
}

static Chemin* creer_chemin(void) {
    Chemin* c = (Chemin*)calloc(1, sizeof(Chemin));
    if (c) { c->cout_total = INFINI; c->bw_minimale = INFINI; c->securite_min = 10; }
    return c;
}

/**
 * @brief Affiche un chemin de façon lisible
 */
void afficher_chemin(Chemin* ch, Graphe* g) {
    if (!ch || ch->longueur == 0) {
        printf("  (aucun chemin trouvé)\n");
        return;
    }
    printf("  Chemin  : ");
    for (int i = 0; i < ch->longueur; i++) {
        if (g) printf("%s", g->noeuds[ch->chemin[i]].nom);
        else   printf("%d", ch->chemin[i]);
        if (i < ch->longueur - 1) printf(" → ");
    }
    printf("\n");
    printf("  Coût    : %.2f\n", ch->cout_total);
    printf("  Latence : %.2f ms\n", ch->latence_totale);
    printf("  BW min  : %.1f Mbps\n", ch->bw_minimale >= INFINI ? 0 : ch->bw_minimale);
    printf("  Séc min : %d/10\n", ch->securite_min);
}

/* ═══════════════════════════════════════════════════════════════════════
   FILE DE PRIORITÉ INTERNE (min-heap tableau) pour Dijkstra
   ═══════════════════════════════════════════════════════════════════════ */
typedef struct { int id; float dist; } HeapNode;
typedef struct {
    HeapNode data[MAX_NOEUDS];
    int      taille;
} MinHeap;

static void heap_push(MinHeap* h, int id, float dist) {
    int i = h->taille++;
    h->data[i].id = id; h->data[i].dist = dist;
    /* sift-up */
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent].dist > h->data[i].dist) {
            HeapNode tmp = h->data[parent];
            h->data[parent] = h->data[i];
            h->data[i] = tmp;
            i = parent;
        } else break;
    }
}

static HeapNode heap_pop(MinHeap* h) {
    HeapNode min = h->data[0];
    h->data[0]   = h->data[--h->taille];
    /* sift-down */
    int i = 0;
    while (1) {
        int l = 2*i+1, r = 2*i+2, smallest = i;
        if (l < h->taille && h->data[l].dist < h->data[smallest].dist) smallest = l;
        if (r < h->taille && h->data[r].dist < h->data[smallest].dist) smallest = r;
        if (smallest == i) break;
        HeapNode tmp = h->data[i]; h->data[i] = h->data[smallest]; h->data[smallest] = tmp;
        i = smallest;
    }
    return min;
}

/* ═══════════════════════════════════════════════════════════════════════
   MODULE 2A — DIJKSTRA
   Complexité : O((V + E) log V)
   ═══════════════════════════════════════════════════════════════════════ */
/**
 * @brief Algorithme de Dijkstra — plus court chemin avec file de priorité
 * @param g      Graphe pondéré (poids ≥ 0 obligatoire)
 * @param source Nœud source
 * @param dest   Nœud destination
 * @param m      Métrique (latence / coût / sécurité)
 * @return Chemin optimal alloué (à libérer par l'appelant), NULL si aucun
 */
Chemin* dijkstra(Graphe* g, int source, int dest, Metrique m) {
    if (!g || !noeud_existe(g, source) || !noeud_existe(g, dest)) return NULL;

    int    V    = g->nb_noeuds;
    float* dist = (float*)malloc(V * sizeof(float));
    int*   prev = (int*)malloc(V * sizeof(int));
    int*   vus  = (int*)calloc(V, sizeof(int));
    if (!dist || !prev || !vus) { free(dist); free(prev); free(vus); return NULL; }

    /* Initialisation */
    for (int i = 0; i < V; i++) { dist[i] = INFINI; prev[i] = -1; }
    dist[source] = 0.0f;

    MinHeap heap; heap.taille = 0;
    heap_push(&heap, source, 0.0f);

    /* Boucle principale */
    while (heap.taille > 0) {
        HeapNode cur = heap_pop(&heap);
        int u = cur.id;
        if (vus[u]) continue;
        vus[u] = 1;
        if (u == dest) break;

        Arete* a = g->noeuds[u].aretes;
        while (a) {
            int   v = a->destination;
            float w = poids_arete(a, m);
            if (!vus[v] && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
                heap_push(&heap, v, dist[v]);
            }
            a = a->suivant;
        }
    }

    /* Reconstruire le chemin */
    Chemin* ch = creer_chemin();
    if (!ch || dist[dest] >= INFINI) {
        free(dist); free(prev); free(vus);
        if (ch) { ch->longueur = 0; }
        return ch;
    }

    /* Remonter prev[] */
    int tmp[MAX_NOEUDS], len = 0;
    for (int v = dest; v != -1; v = prev[v]) tmp[len++] = v;
    ch->longueur = len;
    ch->cout_total = dist[dest];
    ch->bw_minimale = INFINI;
    ch->securite_min = 10;

    for (int i = 0; i < len; i++) ch->chemin[i] = tmp[len - 1 - i];

    /* Calculer latence totale, BW min, sécurité min */
    for (int i = 0; i < len - 1; i++) {
        Arete* a = get_arete(g, ch->chemin[i], ch->chemin[i+1]);
        if (a) {
            ch->latence_totale += a->latence;
            if (a->bande_passante < ch->bw_minimale) ch->bw_minimale = a->bande_passante;
            if (a->securite < ch->securite_min) ch->securite_min = a->securite;
        }
    }

    free(dist); free(prev); free(vus);
    return ch;
}

/* ═══════════════════════════════════════════════════════════════════════
   MODULE 2B — BELLMAN-FORD
   Complexité : O(V · E)
   Avantage   : supporte les poids négatifs
   ═══════════════════════════════════════════════════════════════════════ */
/**
 * @brief Bellman-Ford — relâchement répété V-1 fois de toutes les arêtes
 * @return Chemin optimal, NULL si cycle négatif détecté
 */
Chemin* bellman_ford(Graphe* g, int source, int dest, Metrique m) {
    if (!g || !noeud_existe(g, source) || !noeud_existe(g, dest)) return NULL;

    int    V    = g->nb_noeuds;
    float* dist = (float*)malloc(V * sizeof(float));
    int*   prev = (int*)malloc(V * sizeof(int));
    if (!dist || !prev) { free(dist); free(prev); return NULL; }

    for (int i = 0; i < V; i++) { dist[i] = INFINI; prev[i] = -1; }
    dist[source] = 0.0f;

    /* V-1 itérations de relâchement */
    for (int iter = 0; iter < V - 1; iter++) {
        int modif = 0;
        for (int u = 0; u < V; u++) {
            if (dist[u] >= INFINI) continue;
            Arete* a = g->noeuds[u].aretes;
            while (a) {
                int   v = a->destination;
                float w = poids_arete(a, m);
                if (dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    prev[v] = u;
                    modif   = 1;
                }
                a = a->suivant;
            }
        }
        if (!modif) break; /* convergence anticipée */
    }

    /* Détection de cycle négatif */
    for (int u = 0; u < V; u++) {
        if (dist[u] >= INFINI) continue;
        Arete* a = g->noeuds[u].aretes;
        while (a) {
            if (dist[u] + poids_arete(a, m) < dist[a->destination]) {
                fprintf(stderr, "[AVERT] Cycle négatif détecté !\n");
                free(dist); free(prev);
                return NULL;
            }
            a = a->suivant;
        }
    }

    /* Reconstruire */
    Chemin* ch = creer_chemin();
    if (!ch || dist[dest] >= INFINI) {
        free(dist); free(prev);
        if (ch) ch->longueur = 0;
        return ch;
    }

    int tmp[MAX_NOEUDS], len = 0;
    for (int v = dest; v != -1; v = prev[v]) {
        tmp[len++] = v;
        if (len > V) break; /* sécurité anti-boucle */
    }
    ch->longueur   = len;
    ch->cout_total = dist[dest];
    ch->bw_minimale  = INFINI;
    ch->securite_min = 10;
    for (int i = 0; i < len; i++) ch->chemin[i] = tmp[len-1-i];

    for (int i = 0; i < len - 1; i++) {
        Arete* a = get_arete(g, ch->chemin[i], ch->chemin[i+1]);
        if (a) {
            ch->latence_totale += a->latence;
            if (a->bande_passante < ch->bw_minimale) ch->bw_minimale = a->bande_passante;
            if (a->securite < ch->securite_min) ch->securite_min = a->securite;
        }
    }

    free(dist); free(prev);
    return ch;
}

/* ═══════════════════════════════════════════════════════════════════════
   MODULE 2C — CHEMIN CONTRAINT (BACKTRACKING)
   Complexité : O(b^d) — élagage par contraintes
   ═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    Graphe*     g;
    Contraintes* c;
    Metrique    m;
    int         dest;
    int         visite[MAX_NOEUDS];
    int         chemin_courant[MAX_NOEUDS];
    int         len_courant;
    float       cout_courant;
    float       bw_courante;
    int         sec_courant;
    float       lat_courante;
    /* Meilleure solution */
    Chemin*     meilleur;
} CtxBT;

static int noeud_exclu(Contraintes* c, int id) {
    for (int i = 0; i < c->nb_exclus; i++)
        if (c->noeuds_exclus[i] == id) return 1;
    return 0;
}

static int tous_obligatoires_visites(CtxBT* ctx) {
    for (int i = 0; i < ctx->c->nb_obligatoires; i++) {
        int obligatoire = ctx->c->noeuds_obligatoires[i];
        int trouve = 0;
        for (int j = 0; j < ctx->len_courant; j++)
            if (ctx->chemin_courant[j] == obligatoire) { trouve = 1; break; }
        if (!trouve) return 0;
    }
    return 1;
}

static void bt_explorer(CtxBT* ctx, int u) {
    /* ── Élaguage ── */
    if (ctx->cout_courant > ctx->c->cout_max) return;
    if (ctx->bw_courante  < ctx->c->bw_min && ctx->len_courant > 1) return;
    if (ctx->sec_courant  < ctx->c->sec_min && ctx->len_courant > 1) return;

    /* ── Solution trouvée ── */
    if (u == ctx->dest && tous_obligatoires_visites(ctx)) {
        if (!ctx->meilleur || ctx->cout_courant < ctx->meilleur->cout_total) {
            if (!ctx->meilleur) ctx->meilleur = creer_chemin();
            memcpy(ctx->meilleur->chemin, ctx->chemin_courant,
                   ctx->len_courant * sizeof(int));
            ctx->meilleur->longueur      = ctx->len_courant;
            ctx->meilleur->cout_total    = ctx->cout_courant;
            ctx->meilleur->latence_totale = ctx->lat_courante;
            ctx->meilleur->bw_minimale   = ctx->bw_courante;
            ctx->meilleur->securite_min  = ctx->sec_courant;
        }
        return;
    }

    /* ── Explorer voisins ── */
    Arete* a = ctx->g->noeuds[u].aretes;
    while (a) {
        int v = a->destination;
        if (!ctx->visite[v] && !noeud_exclu(ctx->c, v)) {
            float nouveau_cout = ctx->cout_courant + poids_arete(a, ctx->m);
            float nouvelle_bw  = (ctx->len_courant == 0) ? a->bande_passante
                                  : (a->bande_passante < ctx->bw_courante
                                     ? a->bande_passante : ctx->bw_courante);
            int   nouveau_sec  = (a->securite < ctx->sec_courant)
                                  ? a->securite : ctx->sec_courant;

            /* Élagage anticipé */
            if (nouveau_cout        <= ctx->c->cout_max &&
                nouvelle_bw         >= ctx->c->bw_min   &&
                nouveau_sec         >= ctx->c->sec_min) {

                ctx->visite[v]     = 1;
                ctx->chemin_courant[ctx->len_courant++] = v;
                float old_cout  = ctx->cout_courant;
                float old_bw    = ctx->bw_courante;
                int   old_sec   = ctx->sec_courant;
                float old_lat   = ctx->lat_courante;

                ctx->cout_courant = nouveau_cout;
                ctx->bw_courante  = nouvelle_bw;
                ctx->sec_courant  = nouveau_sec;
                ctx->lat_courante += a->latence;

                bt_explorer(ctx, v);

                /* Dépiler */
                ctx->len_courant--;
                ctx->cout_courant = old_cout;
                ctx->bw_courante  = old_bw;
                ctx->sec_courant  = old_sec;
                ctx->lat_courante = old_lat;
                ctx->visite[v]    = 0;
            }
        }
        a = a->suivant;
    }
}

/**
 * @brief Chemin avec contraintes multiples — backtracking élagué
 */
Chemin* chemin_contraint(Graphe* g, int src, int dest,
                          Contraintes* c, Metrique m) {
    if (!g || !c || !noeud_existe(g, src) || !noeud_existe(g, dest)) return NULL;

    CtxBT ctx;
    memset(&ctx, 0, sizeof(CtxBT));
    ctx.g           = g;
    ctx.c           = c;
    ctx.m           = m;
    ctx.dest        = dest;
    ctx.bw_courante  = INFINI;
    ctx.sec_courant  = 10;
    ctx.meilleur    = NULL;

    ctx.visite[src]     = 1;
    ctx.chemin_courant[ctx.len_courant++] = src;

    bt_explorer(&ctx, src);
    return ctx.meilleur;
}

/* ═══════════════════════════════════════════════════════════════════════
   MODULE 2D — K PLUS COURTS CHEMINS (Yen's algorithm simplifié)
   Approche : diviser-régner — k appels Dijkstra avec exclusions
   ═══════════════════════════════════════════════════════════════════════ */
/**exs-d6h7r1k50q8c73ae7nhg
 * @brief Trouve et affiche les K meilleurs chemins alternatifs
 * @complexity O(k · (V+E) log V)
 */
void k_plus_courts_chemins(Graphe* g, int src, int dest, int k, Metrique m) {
    if (!g || k <= 0) return;
    printf("\n  ══ %d plus courts chemins de %d → %d ══\n", k, src, dest);

    /* On utilise une stratégie simple : Dijkstra répété avec pénalités sur
       les arêtes déjà utilisées (version approchée, suffisant pour le projet) */
    float penalites[MAX_NOEUDS][MAX_NOEUDS];
    for (int i = 0; i < g->nb_noeuds; i++)
        for (int j = 0; j < g->nb_noeuds; j++)
            penalites[i][j] = 0.0f;

    int trouves = 0;
    for (int iter = 0; iter < k * 3 && trouves < k; iter++) {
        /* Appliquer les pénalités temporairement */
        for (int u = 0; u < g->nb_noeuds; u++) {
            Arete* a = g->noeuds[u].aretes;
            while (a) {
                a->latence += penalites[u][a->destination];
                a = a->suivant;
            }
        }

        Chemin* ch = dijkstra(g, src, dest, m);

        /* Retirer les pénalités */
        for (int u = 0; u < g->nb_noeuds; u++) {
            Arete* a = g->noeuds[u].aretes;
            while (a) {
                a->latence -= penalites[u][a->destination];
                a = a->suivant;
            }
        }

        if (ch && ch->longueur > 0) {
            printf("\n  Chemin #%d :\n  ", trouves + 1);
            afficher_chemin(ch, g);

            /* Pénaliser les arêtes de ce chemin pour forcer la diversité */
            for (int i = 0; i < ch->longueur - 1; i++)
                penalites[ch->chemin[i]][ch->chemin[i+1]] += 1000.0f * (iter + 1);

            trouves++;
        }
        free(ch);
    }
    if (trouves == 0) printf("  Aucun chemin trouvé.\n");
}
