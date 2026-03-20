#ifndef LISTE_CHAINEE_H
#define LISTE_CHAINEE_H

/* ─── Paquet réseau ──────────────────────────────────────────────────── */
typedef struct Paquet {
    int   id;
    int   priorite;      /* plus petit = plus prioritaire (min-heap logique) */
    float taille_Mo;
    int   source;
    int   destination;
    float temps_arrivee; /* pour calcul temps d'attente */
    struct Paquet* precedent;
    struct Paquet* suivant;
} Paquet;

/* ─── File d'attente à priorité (liste doublement chaînée triée) ─────── */
typedef struct FileAttente {
    Paquet* tete;
    Paquet* queue;
    int     taille_actuelle;
    int     capacite_max;
    /* Statistiques */
    int     total_enfiles;
    int     total_defiles;
    int     total_perdus;
    float   somme_attente; /* pour calcul moyenne */
} FileAttente;

/* ─── API ────────────────────────────────────────────────────────────── */
FileAttente* creer_file(int capacite_max);
int          enqueue(FileAttente* f, int id, int priorite,
                     float taille, int src, int dest, float temps);
Paquet*      dequeue(FileAttente* f);
Paquet*      peek(FileAttente* f);
int          est_vide_file(FileAttente* f);
void         afficher_file(FileAttente* f);
void         afficher_stats_file(FileAttente* f);
void         liberer_file(FileAttente* f);

#endif /* LISTE_CHAINEE_H */
