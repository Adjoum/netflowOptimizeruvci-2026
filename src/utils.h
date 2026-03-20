#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>

/* Chronomètre en millisecondes */
double chrono_debut(void);
double chrono_fin(double debut);

/* Affichage d'une barre de progression */
void afficher_barre(const char* label, float valeur, float max, int largeur);

/* Génération d'un graphe aléatoire */
#include "graphe.h"
Graphe* graphe_aleatoire(int nb_noeuds, int nb_aretes, int oriente);

/* Nettoyage terminal */
void afficher_titre(const char* titre);
void separateur(void);

#endif
