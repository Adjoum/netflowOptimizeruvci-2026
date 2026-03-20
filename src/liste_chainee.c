/**
 * @file liste_chainee.c
 * @brief File de priorité avec liste doublement chaînée triée
 * @complexity enqueue O(n), dequeue O(1), peek O(1)
 */
#include <stdio.h>
#include <stdlib.h>
#include "liste_chainee.h"

/* ─── Création ───────────────────────────────────────────────────────── */
FileAttente* creer_file(int capacite_max) {
    FileAttente* f = (FileAttente*)malloc(sizeof(FileAttente));
    if (!f) { perror("malloc FileAttente"); return NULL; }
    f->tete             = NULL;
    f->queue            = NULL;
    f->taille_actuelle  = 0;
    f->capacite_max     = capacite_max;
    f->total_enfiles    = 0;
    f->total_defiles    = 0;
    f->total_perdus     = 0;
    f->somme_attente    = 0.0f;
    return f;
}

/* ─── Enqueue : insertion triée par priorité ─────────────────────────── */
/**
 * @brief Insère un paquet dans la file selon sa priorité (tri croissant)
 * @return 1 si succès, 0 si file pleine (paquet perdu)
 * @complexity O(n) — parcours jusqu'à la bonne position
 */
int enqueue(FileAttente* f, int id, int priorite,
            float taille, int src, int dest, float temps) {
    if (!f) return 0;

    /* File pleine → paquet perdu */
    if (f->capacite_max > 0 && f->taille_actuelle >= f->capacite_max) {
        f->total_perdus++;
        printf("[FILE] Paquet %d perdu (file pleine %d/%d)\n",
               id, f->taille_actuelle, f->capacite_max);
        return 0;
    }

    Paquet* p = (Paquet*)malloc(sizeof(Paquet));
    if (!p) { perror("malloc Paquet"); return 0; }
    p->id           = id;
    p->priorite     = priorite;
    p->taille_Mo    = taille;
    p->source       = src;
    p->destination  = dest;
    p->temps_arrivee = temps;
    p->precedent    = NULL;
    p->suivant      = NULL;

    f->total_enfiles++;
    f->taille_actuelle++;

    /* File vide */
    if (!f->tete) {
        f->tete = f->queue = p;
        return 1;
    }

    /* Trouver la position (ordre croissant de priorité) */
    Paquet* cur = f->tete;
    while (cur && cur->priorite <= priorite)
        cur = cur->suivant;

    if (!cur) {
        /* Insérer en queue */
        p->precedent   = f->queue;
        f->queue->suivant = p;
        f->queue       = p;
    } else if (!cur->precedent) {
        /* Insérer en tête */
        p->suivant     = f->tete;
        f->tete->precedent = p;
        f->tete        = p;
    } else {
        /* Insérer au milieu */
        p->precedent   = cur->precedent;
        p->suivant     = cur;
        cur->precedent->suivant = p;
        cur->precedent = p;
    }
    return 1;
}

/* ─── Dequeue : retrait en tête (priorité max) ───────────────────────── */
/**
 * @complexity O(1)
 */
Paquet* dequeue(FileAttente* f) {
    if (!f || !f->tete) return NULL;

    Paquet* p = f->tete;
    f->tete   = f->tete->suivant;
    if (f->tete) f->tete->precedent = NULL;
    else         f->queue = NULL;

    p->suivant   = NULL;
    p->precedent = NULL;
    f->taille_actuelle--;
    f->total_defiles++;
    return p;
}

/* ─── Peek : consulter sans retirer ──────────────────────────────────── */
Paquet* peek(FileAttente* f) {
    return (f) ? f->tete : NULL;
}

int est_vide_file(FileAttente* f) {
    return (!f || f->tete == NULL);
}

/* ─── Affichage ──────────────────────────────────────────────────────── */
void afficher_file(FileAttente* f) {
    if (!f) return;
    printf("\n  File (%d/%d) : ",
           f->taille_actuelle,
           f->capacite_max > 0 ? f->capacite_max : 999);
    Paquet* p = f->tete;
    if (!p) { printf("(vide)\n"); return; }
    while (p) {
        printf("[P%d|prio=%d|%.1fMo|%d→%d]",
               p->id, p->priorite, p->taille_Mo,
               p->source, p->destination);
        if (p->suivant) printf(" ← ");
        p = p->suivant;
    }
    printf("\n");
}

/* ─── Statistiques ───────────────────────────────────────────────────── */
void afficher_stats_file(FileAttente* f) {
    if (!f) return;
    printf("\n  ┌─ Statistiques file d'attente ─────────────┐\n");
    printf("  │  Paquets enfilés  : %-6d                  │\n", f->total_enfiles);
    printf("  │  Paquets défilés  : %-6d                  │\n", f->total_defiles);
    printf("  │  Paquets perdus   : %-6d                  │\n", f->total_perdus);
    printf("  │  En attente       : %-6d                  │\n", f->taille_actuelle);
    if (f->total_defiles > 0)
        printf("  │  Taux de perte    : %.1f%%                   │\n",
               100.0f * f->total_perdus / (f->total_enfiles > 0 ? f->total_enfiles : 1));
    printf("  └────────────────────────────────────────────┘\n");
}

/* ─── Libération ─────────────────────────────────────────────────────── */
void liberer_file(FileAttente* f) {
    if (!f) return;
    Paquet* p = f->tete;
    while (p) {
        Paquet* tmp = p->suivant;
        free(p);
        p = tmp;
    }
    free(f);
}
