#ifndef GRAPHE_H
#define GRAPHE_H

#define NOM_MAX       50
#define INFINI        1e9f
#define MAX_NOEUDS    500

/* ─── Arête (liste d'adjacence) ─────────────────────────────────────── */
typedef struct Arete {
    int   destination;
    float latence;        /* ms          */
    float bande_passante; /* Mbps        */
    float cout;           /* unité moné. */
    int   securite;       /* 0-10        */
    struct Arete* suivant;
} Arete;

/* ─── Nœud ───────────────────────────────────────────────────────────── */
typedef struct Noeud {
    int   id;
    char  nom[NOM_MAX];
    Arete* aretes;        /* liste d'adjacence */
} Noeud;

/* ─── Graphe ─────────────────────────────────────────────────────────── */
typedef struct Graphe {
    int    nb_noeuds;
    int    oriente;       /* 1 = orienté, 0 = non orienté */
    Noeud* noeuds;
    float** matrice_adj;  /* représentation alternative   */
} Graphe;

/* ─── API ────────────────────────────────────────────────────────────── */
Graphe* creer_graphe(int nb_noeuds, int oriente);
void    ajouter_arete(Graphe* g, int src, int dest,
                      float lat, float bw, float cout, int sec);
void    supprimer_arete(Graphe* g, int src, int dest);
void    afficher_graphe(Graphe* g);
void    charger_graphe(Graphe** g, const char* fichier);
void    sauvegarder_graphe(Graphe* g, const char* fichier);
void    liberer_graphe(Graphe* g);
int     noeud_existe(Graphe* g, int id);
Arete*  get_arete(Graphe* g, int src, int dest);

#endif /* GRAPHE_H */
