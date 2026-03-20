#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "utils.h"
#include "graphe.h"

#ifdef _WIN32
#include <windows.h>

double chrono_debut(void) {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart;
}

double chrono_fin(double debut) {
    LARGE_INTEGER t, freq;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&freq);
    return ((double)t.QuadPart - debut) / (double)freq.QuadPart * 1000.0;
}

#else
/* Linux/Mac — clock_gettime nanoseconde */
double chrono_debut(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1.0e6;
}

double chrono_fin(double debut) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1.0e6 - debut;
}
#endif

void afficher_barre(const char* label, float valeur, float max, int largeur) {
    int rempli = (int)(largeur * valeur / (max > 0 ? max : 1));
    printf("  %-20s [", label);
    for (int i = 0; i < largeur; i++) printf(i < rempli ? "█" : "░");
    printf("] %.1f\n", valeur);
}

void afficher_titre(const char* titre) {
    int len = (int)strlen(titre) + 4;
    printf("\n╔");
    for (int i = 0; i < len; i++) printf("═");
    printf("╗\n║  %s  ║\n╚", titre);
    for (int i = 0; i < len; i++) printf("═");
    printf("╝\n");
}

void separateur(void) {
    printf("  ──────────────────────────────────────────────\n");
}



Graphe* graphe_aleatoire(int nb_noeuds, int nb_aretes, int oriente) {
    /* PAS de srand ici — géré par l'appelant */
    Graphe* g = creer_graphe(nb_noeuds, oriente);
    if (!g) return NULL;
    int tentatives = 0, crees = 0;
    while (crees < nb_aretes && tentatives < nb_aretes * 10) {
        int src  = rand() % nb_noeuds;
        int dest = rand() % nb_noeuds;
        if (src == dest || get_arete(g, src, dest)) { tentatives++; continue; }
        float lat  = 1.0f  + (float)(rand() % 100);
        float bw   = 10.0f + (float)(rand() % 990);
        float cout = 0.5f  + (float)(rand() % 20) * 0.5f;
        int   sec  = 1 + rand() % 10;
        ajouter_arete(g, src, dest, lat, bw, cout, sec);
        crees++; tentatives++;
    }
    return g;
}
/*Graphe* graphe_aleatoire(int nb_noeuds, int nb_aretes, int oriente) {
    srand((unsigned)time(NULL));
    Graphe* g = creer_graphe(nb_noeuds, oriente);
    if (!g) return NULL;
    int tentatives = 0;
    int crees = 0;
    while (crees < nb_aretes && tentatives < nb_aretes * 10) {
        int src  = rand() % nb_noeuds;
        int dest = rand() % nb_noeuds;
        if (src == dest) { tentatives++; continue; }
        if (get_arete(g, src, dest)) { tentatives++; continue; }
        float lat  = 1.0f  + (float)(rand() % 100);
        float bw   = 10.0f + (float)(rand() % 990);
        float cout = 0.5f  + (float)(rand() % 20) * 0.5f;
        int   sec  = 1 + rand() % 10;
        ajouter_arete(g, src, dest, lat, bw, cout, sec);
        crees++;
        tentatives++;
    }
    return g;
}
   */












/*#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "utils.h"
#include "graphe.h"

double chrono_debut(void) {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}
double chrono_fin(double debut) {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0 - debut;
}  

void afficher_barre(const char* label, float valeur, float max, int largeur) {
    int rempli = (int)(largeur * valeur / (max > 0 ? max : 1));
    printf("  %-20s [", label);
    for (int i = 0; i < largeur; i++) printf(i < rempli ? "█" : "░");
    printf("] %.1f\n", valeur);
}

void afficher_titre(const char* titre) {
    int len = (int)strlen(titre) + 4;
    printf("\n╔");
    for (int i = 0; i < len; i++) printf("═");
    printf("╗\n║  %s  ║\n╚", titre);
    for (int i = 0; i < len; i++) printf("═");
    printf("╝\n");
}

void separateur(void) {
    printf("  ──────────────────────────────────────────────\n");
}

Graphe* graphe_aleatoire(int nb_noeuds, int nb_aretes, int oriente) {
    srand((unsigned)time(NULL));
    Graphe* g = creer_graphe(nb_noeuds, oriente);
    if (!g) return NULL;
    int tentatives = 0;
    int crees = 0;
    while (crees < nb_aretes && tentatives < nb_aretes * 10) {
        int src  = rand() % nb_noeuds;
        int dest = rand() % nb_noeuds;
        if (src == dest) { tentatives++; continue; }
        if (get_arete(g, src, dest)) { tentatives++; continue; }
        float lat = 1.0f + (float)(rand() % 100);
        float bw  = 10.0f + (float)(rand() % 990);
        float cout = 0.5f + (float)(rand() % 20) * 0.5f;
        int   sec  = 1 + rand() % 10;
        ajouter_arete(g, src, dest, lat, bw, cout, sec);
        crees++;
        tentatives++;
    }
    return g;
}
             */