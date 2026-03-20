/**
 * @file tests_unitaires.c
 * @brief Tests unitaires — NetFlow Optimizer
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "../src/graphe.h"
#include "../src/liste_chainee.h"
#include "../src/dijkstra.h"
#include "../src/securite.h"

static int tests_ok = 0, tests_total = 0;

#define TEST(nom, condition) do { \
    tests_total++; \
    if (condition) { printf("  [✓] %s\n", nom); tests_ok++; } \
    else           { printf("  [✗] %s — ÉCHEC (ligne %d)\n", nom, __LINE__); } \
} while(0)

/* ─── Graphe ─────────────────────────────────────────────────────────── */
static void test_graphe(void) {
    printf("\n── Tests Graphe ────────────────────────────\n");
    Graphe* g = creer_graphe(5, 1);
    TEST("Création graphe",   g != NULL);
    TEST("Nb nœuds correct",  g->nb_noeuds == 5);
    TEST("Orienté",           g->oriente == 1);

    ajouter_arete(g, 0, 1, 10.0f, 100.0f, 2.0f, 8);
    ajouter_arete(g, 1, 2, 5.0f,  200.0f, 1.0f, 9);
    ajouter_arete(g, 0, 3, 20.0f, 50.0f,  3.0f, 7);
    ajouter_arete(g, 3, 4, 8.0f,  150.0f, 2.5f, 6);

    TEST("Arête 0→1 créée",   get_arete(g, 0, 1) != NULL);
    TEST("Arête 0→1 latence", fabsf(get_arete(g, 0, 1)->latence - 10.0f) < 0.01f);
    TEST("Arête 1→0 absente (orienté)", get_arete(g, 1, 0) == NULL);
    TEST("Matrice mise à jour", fabsf(g->matrice_adj[0][1] - 10.0f) < 0.01f);

    supprimer_arete(g, 0, 1);
    TEST("Suppression arête 0→1", get_arete(g, 0, 1) == NULL);

    liberer_graphe(g);
    TEST("Libération OK", 1); /* pas de crash */

    /* Graphe non orienté */
    Graphe* g2 = creer_graphe(3, 0);
    ajouter_arete(g2, 0, 1, 5.0f, 100.0f, 1.0f, 9);
    TEST("Non orienté : arête symétrique", get_arete(g2, 1, 0) != NULL);
    liberer_graphe(g2);
}

/* ─── Liste chaînée / File ───────────────────────────────────────────── */
static void test_file(void) {
    printf("\n── Tests File de Priorité ──────────────────\n");
    FileAttente* f = creer_file(5);
    TEST("Création file",  f != NULL);
    TEST("File vide init", est_vide_file(f));

    enqueue(f, 1, 5, 1.0f, 0, 3, 0.0f);
    enqueue(f, 2, 1, 2.0f, 1, 2, 0.0f);
    enqueue(f, 3, 3, 0.5f, 0, 4, 0.0f);

    TEST("Taille après 3 enfilages", f->taille_actuelle == 3);

    Paquet* p = peek(f);
    TEST("Peek = priorité 1 (la plus haute)", p && p->priorite == 1);

    Paquet* d = dequeue(f);
    TEST("Dequeue : priorité 1 sort en premier", d && d->priorite == 1);
    free(d);

    d = dequeue(f);
    TEST("Dequeue : priorité 3 sort ensuite", d && d->priorite == 3);
    free(d);

    /* Test file pleine — apres 2 dequeue il reste 1 paquet, on ajoute 4 */
    enqueue(f, 4, 2, 1.0f, 0, 1, 0.0f);
    enqueue(f, 5, 4, 1.0f, 0, 1, 0.0f);
    enqueue(f, 6, 6, 1.0f, 0, 1, 0.0f);
    enqueue(f, 7, 8, 1.0f, 0, 1, 0.0f);
    TEST("File pleine a capacite 5", f->taille_actuelle == 5);
    int perdu_avant = f->total_perdus;
    enqueue(f, 8, 9, 1.0f, 0, 1, 0.0f);
    TEST("Paquet perdu si file pleine", f->total_perdus == perdu_avant + 1);

    liberer_file(f);
}

/* ─── Dijkstra ───────────────────────────────────────────────────────── */
static void test_dijkstra(void) {
    printf("\n── Tests Dijkstra ──────────────────────────\n");
    /*
     *  0 --10--> 1 --5--> 3
     *  |         |
     *  20        15
     *  ↓         ↓
     *  2 --8---> 4
     *
     *  Plus court 0→3 = 0→1→3 = 15
     *  Plus court 0→4 = 0→1→4 = 25 ou 0→2→4 = 28
     */
    Graphe* g = creer_graphe(5, 1);
    ajouter_arete(g, 0, 1, 10.0f, 100.0f, 1.0f, 9);
    ajouter_arete(g, 0, 2, 20.0f, 100.0f, 1.0f, 9);
    ajouter_arete(g, 1, 3, 5.0f,  100.0f, 1.0f, 9);
    ajouter_arete(g, 1, 4, 15.0f, 100.0f, 1.0f, 9);
    ajouter_arete(g, 2, 4, 8.0f,  100.0f, 1.0f, 9);

    Chemin* ch = dijkstra(g, 0, 3, METRIQUE_LATENCE);
    TEST("Dijkstra : chemin 0→3 trouvé",     ch && ch->longueur > 0);
    TEST("Dijkstra : coût 0→3 = 15.0",       ch && fabsf(ch->cout_total - 15.0f) < 0.01f);
    TEST("Dijkstra : chemin passe par 1",    ch && ch->longueur == 3 && ch->chemin[1] == 1);
    free(ch);

    ch = dijkstra(g, 0, 4, METRIQUE_LATENCE);
    TEST("Dijkstra : coût 0→4 = 25.0",       ch && fabsf(ch->cout_total - 25.0f) < 0.01f);
    free(ch);

    ch = dijkstra(g, 3, 0, METRIQUE_LATENCE); /* orienté : pas de retour */
    TEST("Dijkstra : pas de chemin 3→0 (orienté)", ch && ch->longueur == 0);
    free(ch);
    liberer_graphe(g);
}

/* ─── Bellman-Ford ───────────────────────────────────────────────────── */
static void test_bellman(void) {
    printf("\n── Tests Bellman-Ford ──────────────────────\n");
    Graphe* g = creer_graphe(4, 1);
    ajouter_arete(g, 0, 1, 10.0f, 100.0f, 1.0f, 9);
    ajouter_arete(g, 0, 2, 6.0f,  100.0f, 1.0f, 9);
    ajouter_arete(g, 1, 3, 5.0f,  100.0f, 1.0f, 9);
    ajouter_arete(g, 2, 1, 1.0f,  100.0f, 1.0f, 9); /* 0→2→1 = 7, mieux que 0→1 = 10 */
    ajouter_arete(g, 2, 3, 20.0f, 100.0f, 1.0f, 9);

    Chemin* ch = bellman_ford(g, 0, 3, METRIQUE_LATENCE);
    TEST("Bellman-Ford : chemin 0→3",        ch && ch->longueur > 0);
    TEST("Bellman-Ford : coût optimal 12.0", ch && fabsf(ch->cout_total - 12.0f) < 0.01f);
    free(ch);
    liberer_graphe(g);
}

/* ─── Sécurité ───────────────────────────────────────────────────────── */
static void test_securite(void) {
    printf("\n── Tests Sécurité ──────────────────────────\n");

    /* Graphe avec cycle */
    Graphe* g = creer_graphe(4, 1);
    ajouter_arete(g, 0, 1, 1, 100, 1, 9);
    ajouter_arete(g, 1, 2, 1, 100, 1, 9);
    ajouter_arete(g, 2, 0, 1, 100, 1, 9); /* cycle 0→1→2→0 */
    ajouter_arete(g, 2, 3, 1, 100, 1, 9);
    TEST("Détection cycle (présent)",   detecter_cycles(g) == 1);
    liberer_graphe(g);

    /* Graphe sans cycle */
    Graphe* g2 = creer_graphe(4, 1);
    ajouter_arete(g2, 0, 1, 1, 100, 1, 9);
    ajouter_arete(g2, 1, 2, 1, 100, 1, 9);
    ajouter_arete(g2, 2, 3, 1, 100, 1, 9);
    TEST("Détection cycle (absent)",    detecter_cycles(g2) == 0);

    /* Composantes connexes */
    ResultatSecurite res; memset(&res, 0, sizeof(res));
    composantes_connexes(g2, &res);
    TEST("Composantes connexes : 1",    res.nb_composantes == 1);
    liberer_graphe(g2);

    /* Graphe non connexe */
    Graphe* g3 = creer_graphe(4, 0);
    ajouter_arete(g3, 0, 1, 1, 100, 1, 9);
    ajouter_arete(g3, 2, 3, 1, 100, 1, 9);
    ResultatSecurite res2; memset(&res2, 0, sizeof(res2));
    composantes_connexes(g3, &res2);
    TEST("Composantes connexes : 2 (non connexe)", res2.nb_composantes == 2);
    liberer_graphe(g3);
}

/* ─── Main tests ─────────────────────────────────────────────────────── */
int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║     TESTS UNITAIRES — NetFlow Optimizer      ║\n");
    printf("╚══════════════════════════════════════════════╝\n");

    test_graphe();
    test_file();
    test_dijkstra();
    test_bellman();
    test_securite();

    printf("\n══════════════════════════════════════════════\n");
    printf("  Résultat : %d/%d tests réussis\n", tests_ok, tests_total);
    if (tests_ok == tests_total)
        printf("  ✅ TOUS LES TESTS PASSÉS\n");
    else
        printf("  ❌ %d TEST(S) ÉCHOUÉ(S)\n", tests_total - tests_ok);
    printf("══════════════════════════════════════════════\n");

    return (tests_ok == tests_total) ? 0 : 1;
}

