/**
 * @file api.c
 * @brief Mode API JSON — NetFlow Optimizer & Security Analyzer
 *
 * Protocole : stdin → commande JSON / stdout → réponse JSON
 * Lancé par Flask avec :  netflow.exe --api
 *
 * ALC2101 — UVCI 2025-2026
 */

#include <stdio.h> /* fopen, fclose, _fileno (MinGW) */
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <io.h> /* _dup, _dup2, _close */
/* MinGW masque fileno et _fileno avec -std=c11 : déclaration explicite */
extern int fileno(FILE*);
#else
#include <unistd.h> /* dup, dup2, close */
#endif

/* tes headers locaux */
#include "api.h"
#include "graphe.h"
#include "dijkstra.h"
#include "securite.h"
#include "liste_chainee.h"
#include "utils.h"


/* ── État global de la session API ────────────────────────────────────── */
static Graphe*      G_api  = NULL;
static FileAttente* FA_api = NULL;
static int          PKT_ID = 1;

/* ══════════════════════════════════════════════════════════════════════
   MINI PARSER JSON (sans dépendance externe)
   ══════════════════════════════════════════════════════════════════════ */
static int jstr(const char* j, const char* k, char* out, int max) {
    char pat[128]; 
    snprintf(pat, sizeof(pat), "\"%s\"", k);
    const char* p = strstr(j, pat); 
    if (!p) return 0;
    p += strlen(pat);
    while (*p==' '||*p==':') p++;
    if (*p != '"') return 0; 
    p++;
    int i=0; 
    while (*p&&*p!='"'&&i<max-1) out[i++]=*p++;
    out[i]='\0'; 
    return 1;
}
static int jint(const char* j, const char* k, int* out) {
    char pat[128]; 
    snprintf(pat, sizeof(pat), "\"%s\"", k);
    const char* p = strstr(j, pat); 
    if (!p) return 0;
    p += strlen(pat);
    while (*p==' '||*p==':') p++;
    if (*p=='-'||(*p>='0'&&*p<='9')) { 
        *out=atoi(p); 
        return 1; }
    return 0;
}
static int jfloat(const char* j, const char* k, float* out) {
    char pat[128]; 
    snprintf(pat, sizeof(pat), "\"%s\"", k);
    const char* p = strstr(j, pat); 
    if (!p) return 0;
    p += strlen(pat);
    while (*p==' '||*p==':') p++;
    if (*p=='-'||(*p>='0'&&*p<='9')) { 
        *out=(float)atof(p); 
        return 1; 
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
   SÉRIALISEURS JSON
   ══════════════════════════════════════════════════════════════════════ */
static Metrique to_metric(const char* s) {
    if (strcmp(s,"cout")==0)     return METRIQUE_COUT;
    if (strcmp(s,"securite")==0) return METRIQUE_SECURITE;
    return METRIQUE_LATENCE;
}

/* Supprime les prints parasites de charger_graphe */
static void silent_load(const char* file) {
/* Redirige stdout vers NUL/dev/null pour masquer les prints de charger_graphe */
    /* Compatible Windows (MinGW) et Linux */
#ifdef _WIN32
    int saved = _dup(1);
    FILE* devnull = fopen("NUL", "w");
    if (devnull) {
        _dup2(fileno(devnull), 1);   /* fileno sans underscore */
        fclose(devnull);
    }
    charger_graphe(&G_api, file);
    fflush(stdout);
    if (saved != -1) {
        _dup2(saved, 1);
        _close(saved);
    }
#else
    int saved = dup(1);
    FILE* devnull = fopen("/dev/null", "w");
    if (devnull) {
        dup2(fileno(devnull), 1);
        fclose(devnull);
    }
    charger_graphe(&G_api, file);
    fflush(stdout);
    if (saved != -1) {
        dup2(saved, 1);
        close(saved);
    }
#endif
}  

//static void silent_load(const char* file) {
    /* Redirige stdout vers NUL/dev/null pour masquer les prints de charger_graphe */
    /* Compatible Windows (MinGW) et Linux */
/*#ifdef _WIN32
    int saved = _dup(1);
    FILE* devnull = fopen("NUL", "w");
    if (devnull) { 
        _dup2(_fileno(devnull), 1); 
        fclose(devnull); 
    }
    charger_graphe(&G_api, file);
    fflush(stdout);
    if (saved != -1) { 
        _dup2(saved, 1); 
        _close(saved); 
    }
#else
    int saved = dup(1);
    FILE* devnull = fopen("/dev/null", "w");
    if (devnull) { 
        dup2(fileno(devnull), 1); 
        fclose(devnull); }
    charger_graphe(&G_api, file);
    fflush(stdout);
    if (saved != -1) { 
        dup2(saved, 1); 
        close(saved); }
#endif
}   */

static void print_graph(void) {
    if (!G_api) { 
        printf("{\"nodes\":[],\"edges\":[],\"directed\":true}"); 
        return; 
    }
    printf("{\"nodes\":[");
    for (
        int i=0;
        i<G_api->nb_noeuds;
        i++) {
        if (i) printf(",");
        printf("{\"id\":%d,\"name\":\"%s\"}", i, G_api->noeuds[i].nom);
    }
    printf("],\"edges\":[");
    int first=1;
    for (int i=0;i<G_api->nb_noeuds;i++) {
        Arete* a=G_api->noeuds[i].aretes;
        while(a) {
            if (!first) printf(",");
            printf("{\"src\":%d,\"dst\":%d,\"lat\":%.2f,\"bw\":%.1f,\"cost\":%.2f,\"sec\":%d}",
                   i,a->destination,a->latence,a->bande_passante,a->cout,a->securite);
            first=0; a=a->suivant;
        }
    }
    printf("],\"directed\":%s,\"nb_noeuds\":%d}",
           G_api->oriente?"true":"false", G_api->nb_noeuds);
}

static void print_chemin(Chemin* ch, double ms) {
    if (!ch||ch->longueur==0) { 
        printf("{\"found\":false,\"path\":[],\"exec_ms\":%.4f}",ms); 
        return; 
    }
    printf("{\"found\":true,\"path\":[");
    for (int i=0;i<ch->longueur;i++) { 
        if(i)printf(","); 
        printf("%d",ch->chemin[i]); 
    }
    printf("],\"cost\":%.4f,\"latence\":%.4f,\"bw_min\":%.1f,\"sec_min\":%d,\"length\":%d,\"exec_ms\":%.4f}",
           ch->cout_total, ch->latence_totale,
           ch->bw_minimale>=1e8f?0:ch->bw_minimale,
           ch->securite_min, ch->longueur, ms);
}

/* ══════════════════════════════════════════════════════════════════════
   TARJAN (version autonome pour sérialisation JSON)
   ══════════════════════════════════════════════════════════════════════ */
static int _disc[MAX_NOEUDS], _low[MAX_NOEUDS], _on[MAX_NOEUDS];
static int _stk[MAX_NOEUDS], _stop=-1, _tmr=0;
static int _scc[MAX_NOEUDS][50], _ssz[MAX_NOEUDS], _snb=0;

static void tarjan_r(Graphe* g, int u) {
    _disc[u]=_low[u]=_tmr++;
    _stk[++_stop]=u; 
    _on[u]=1;
    Arete* a=g->noeuds[u].aretes;
    while(a) {
        int v=a->destination;
        if(_disc[v]==-1) { 
            tarjan_r(g,v); 
            if(_low[v]<_low[u])_low[u]=_low[v]; 
        }
        else if(_on[v])  { if(_disc[v]<_low[u])_low[u]=_disc[v]; }
        a=a->suivant;
    }
    if(_low[u]==_disc[u]) {
        int sz=0,v;
        do { v=_stk[_stop--]; 
            _on[v]=0;
            if(sz<50) _scc[_snb][sz++]=v;
        } while(v!=u);
        _ssz[_snb++]=sz;
    }
}

/* ══════════════════════════════════════════════════════════════════════
   HANDLERS DE COMMANDES
   ══════════════════════════════════════════════════════════════════════ */
static void do_dijkstra(const char* j) {
    int src=0,dst=1; char ms[32]="latence";
    jint(j,"src",&src); 
    jint(j,"dst",&dst); 
    jstr(j,"metric",ms,sizeof(ms));
    double t0=chrono_debut();
    Chemin* ch=G_api?dijkstra(G_api,src,dst,to_metric(ms)):NULL;
    print_chemin(ch,chrono_fin(t0)); 
    free(ch);
}

static void do_bellman(const char* j) {
    int src=0,dst=1; char ms[32]="latence";
    jint(j,"src",&src); jint(j,"dst",&dst); 
    jstr(j,"metric",ms,sizeof(ms));
    double t0=chrono_debut();
    Chemin* ch=G_api?bellman_ford(G_api,src,dst,to_metric(ms)):NULL;
    print_chemin(ch,chrono_fin(t0)); 
    free(ch);
}

static void do_compare(const char* j) {
    int src=0,dst=1; 
    char ms[32]="latence";
    jint(j,"src",&src); 
    jint(j,"dst",&dst); jstr(j,"metric",ms,sizeof(ms));
    Metrique m=to_metric(ms);
    double t0=chrono_debut(); 
    Chemin* cd=G_api?dijkstra(G_api,src,dst,m):NULL; 
    double td=chrono_fin(t0);
    double t1=chrono_debut(); 
    Chemin* cb=G_api?bellman_ford(G_api,src,dst,m):NULL; 
    double tb=chrono_fin(t1);
    printf("{\"dijkstra\":"); 
    print_chemin(cd,td);
    printf(",\"bellman\":"); 
    print_chemin(cb,tb);
    printf(",\"ratio\":%.4f}", td>0.0001?tb/td:0.0);
    free(cd); 
    free(cb);
}

static void do_kpaths(const char* j) {
    int src=0,dst=1,k=3; 
    char ms[32]="latence";
    jint(j,"src",&src); 
    jint(j,"dst",&dst); 
    jint(j,"k",&k); 
    jstr(j,"metric",ms,sizeof(ms));

    if(!G_api){

        printf("{\"paths\":[]}");
        return;
    }
    /* Pénalités pour forcer la diversité */
    static float pen[MAX_NOEUDS][MAX_NOEUDS];
    memset(pen,0,sizeof(pen));
    printf("{\"paths\":["); 
    int found=0;

    for(int iter=0;iter<k*3&&found<k;iter++){

        for(
            int u=0;
            u<G_api->nb_noeuds;
            u++){

            Arete*a=G_api->noeuds[u].aretes;

            while(a){a->latence+=pen[u][a->destination];

                a=a->suivant;
            }
        }
        Chemin* ch=dijkstra(G_api,src,dst,to_metric(ms));
        for(
            int u=0;
            u<G_api->nb_noeuds;
            u++){
            Arete*a=G_api->noeuds[u].aretes;
            while(a){a->latence-=pen[u][a->destination];
                a=a->suivant;
            }
        }
        if(ch&&ch->longueur>0){
            if(found)printf(","); 
            print_chemin(ch,0);
            for(int i=0;
                i<ch->longueur-1;i++) pen[ch->chemin[i]][ch->chemin[i+1]]+=1000.0f*(iter+1);
            found++;
        }
        free(ch);
    }
    printf("]}");
}

static void do_constrained(const char* j) {
    int src=0,dst=1,sec_min=0;
    float bw_min=0,cout_max=999999;
    jint(j,"src",&src); 
    jint(j,"dst",&dst); 
    jint(j,"sec_min",&sec_min);
    jfloat(j,"bw_min",&bw_min); 
    jfloat(j,"cout_max",&cout_max);
    Contraintes c; 
    memset(&c,0,sizeof(c));
    c.bw_min=bw_min; 
    c.cout_max=cout_max>0?cout_max:999999.0f; 
    c.sec_min=sec_min;
    double t0=chrono_debut();
    Chemin* ch=G_api?chemin_contraint(G_api,src,dst,&c,METRIQUE_LATENCE):NULL;
    print_chemin(ch,chrono_fin(t0)); 
    free(ch);
}

static void do_dfs(const char* j) {
    int start=0; 
    jint(j,"start",&start);
    if(!G_api||!noeud_existe(G_api,start)){
        printf("{\"order\":[],\"visited\":0,\"total\":0}");
        return;
    }
    int vis[MAX_NOEUDS]={0}, stk[MAX_NOEUDS], top=-1, ord[MAX_NOEUDS], len=0;
    stk[++top]=start;
    while(top>=0){
        int u=stk[top--]; 
        if(vis[u])continue; 
        vis[u]=1; 
        ord[len++]=u;
        Arete* nb[MAX_NOEUDS]; 
        int nb_cnt=0;
        Arete* a=G_api->noeuds[u].aretes; 
        while(a){
            nb[nb_cnt++]=a;
            a=a->suivant;
        }
        for(
            int i=nb_cnt-1;
            i>=0;
            i--) if(!vis[nb[i]->destination]) stk[++top]=nb[i]->destination;
    }
    printf("{\"order\":[");
    for(int i=0;i<len;i++){if(i)printf(",");printf("%d",ord[i]);}
    printf("],\"visited\":%d,\"total\":%d}",len,G_api->nb_noeuds);
}

static void do_bfs(const char* j) {
    int start=0; jint(j,"start",&start);
    if(!G_api||!noeud_existe(G_api,start)){
        printf("{\"order\":[],\"visited\":0,\"total\":0}");
        return;
    }
    int vis[MAX_NOEUDS]={0}, niv[MAX_NOEUDS], q[MAX_NOEUDS], head=0, tail=0;
    int ord[MAX_NOEUDS], len=0;
    memset(niv,-1,sizeof(niv));
    vis[start]=1; 
    niv[start]=0; 
    q[tail++]=start;
    while(head<tail){
        int u=q[head++]; 
        ord[len++]=u;
        Arete* a=G_api->noeuds[u].aretes;
        while(a){
            if(!vis[a->destination]){
                vis[a->destination]=1;niv[a->destination]=niv[u]+1;
                q[tail++]=a->destination;
            }
            a=a->suivant;
        }
    }
    printf("{\"order\":[");
    for(
        int i=0;
        i<len;
        i++
    ){
        if(i)printf(",");
        printf("{\"id\":%d,\"level\":%d}",ord[i],niv[ord[i]]);
    }
    printf("],\"visited\":%d,\"total\":%d}",len,G_api->nb_noeuds);
}

static void do_security_full(void) {
    if(!G_api){
        printf("{\"ok\":false}");
        return;
    }
    int cy=detecter_cycles(G_api);
    ResultatSecurite res; 
    memset(&res,0,sizeof(res));
    points_articulation(G_api,&res);
    composantes_connexes(G_api,&res);
    memset(_disc,-1,sizeof(_disc)); 
    memset(_low,0,sizeof(_low)); 
    memset(_on,0,sizeof(_on));
    _stop=-1; 
    _tmr=0; 
    _snb=0;
    for(
        int i=0;
        i<G_api->nb_noeuds;
        i++
    ) 
    if(_disc[i]==-1) tarjan_r(G_api,i);
    int low_sec=0;
    for(
        int i=0;
        i<G_api->nb_noeuds;
        i++){
            Arete*a=G_api->noeuds[i].aretes;
            while(a){
                if(a->securite<5)low_sec++;
                a=a->suivant;
            }
        }
    printf("{\"has_cycle\":%s",cy?"true":"false");
    printf(",\"articulation_count\":%d,\"articulations\":[",res.nb_points_articulation);
    for(
        int i=0;
        i<res.nb_points_articulation;
        i++
    )
    {if(i)printf(",");
        printf("%d",res.points_articulation[i]);
    }
    printf("],\"bridge_count\":%d,\"bridges\":[",res.nb_ponts);
    for(
        int i=0;
        i<res.nb_ponts;
        i++
    ){
        if(i)printf(",");
        printf("{\"src\":%d,\"dst\":%d}",res.ponts_src[i],res.ponts_dest[i]);
    }
    printf("],\"component_count\":%d,\"scc_count\":%d,\"low_sec_edges\":%d}",
           res.nb_composantes,_snb,low_sec);
}

static void do_tarjan(void) {
    if(!G_api){
        
        printf("{\"sccs\":[],\"count\":0}");
        return;
    }
    memset(_disc,-1,sizeof(_disc));
    memset(_low,0,sizeof(_low)); 
    memset(_on,0,sizeof(_on));
    _stop=-1; 
    _tmr=0; 
    _snb=0;

    for(

        int i=0;
        i<G_api->nb_noeuds;
        i++) 
        if(_disc[i]==-1) tarjan_r(G_api,i);

    printf("{\"sccs\":[");
    for(int s=0;s<_snb;s++){
        if(s)printf(","); 
        printf("[");
        for(
            int j=0;
            j<_ssz[s];
            j++
        ){
            if(j)printf(",");
            printf("%d",_scc[s][j]);
        }
        printf("]");
    }
    printf("],\"count\":%d}",_snb);
}

static void do_benchmark(void) {
    int sizes[]={10,25,50,100,200}; 
    int nb=5;
    srand(42);
    printf("{\"results\":[");
    for(
        int t=0;
        t<nb;
        t++){
        int n=sizes[t];
        Graphe* g=graphe_aleatoire(n,n*3,1); 
        if(!g)continue;
        double td=0,tb=0; 
        int reps=3;
        for(int r=0;r<reps;r++){
            double t0=chrono_debut(); 
            Chemin* cd=dijkstra(g,0,n-1,METRIQUE_LATENCE); 
            td+=chrono_fin(t0); 
            free(cd);
            t0=chrono_debut(); 
            Chemin* cb=bellman_ford(g,0,n-1,METRIQUE_LATENCE); 
            tb+=chrono_fin(t0); 
            free(cb);
        }
        liberer_graphe(g);
        if(t)printf(",");
        printf("{\"n\":%d,\"edges\":%d,\"dijkstra_ms\":%.4f,\"bellman_ms\":%.4f,\"ratio\":%.3f}",
               n,n*3,td/reps,tb/reps,td>0.0001?(tb/reps)/(td/reps):0.0);
    }
    printf("]}");
}

static void do_queue(const char* j) {
    char op[32]="list"; 
    jstr(j,"op",op,sizeof(op));
    if(!FA_api){
        int cap=20;
        jint(j,"cap",&cap);
        FA_api=creer_file(cap);
    }
    if(strcmp(op,"enqueue")==0){
        int src=0,dst=1,prio=5; 
        float sz=1.0f;
        jint(j,"src",&src);
        jint(j,"dst",&dst);
        jint(j,"prio",&prio);
        jfloat(j,"size",&sz);
        int ok=enqueue(FA_api,PKT_ID++,prio,sz,src,dst,0.0f);
        printf("{\"ok\":%s,\"id\":%d,\"taille\":%d,\"perdus\":%d}",ok?"true":"false",PKT_ID-1,FA_api->taille_actuelle,FA_api->total_perdus);
    } else if(strcmp(op,"dequeue")==0){
        Paquet* p=dequeue(FA_api);
        if(p){
            printf("{\"ok\":true,\"id\":%d,\"prio\":%d,\"src\":%d,\"dst\":%d,\"size\":%.1f}",p->id,p->priorite,p->source,p->destination,p->taille_Mo);
            free(p);
        }
        else printf("{\"ok\":false,\"message\":\"File vide\"}");
    } else if(strcmp(op,"peek")==0){
        Paquet* p=peek(FA_api);
        if(p) printf("{\"ok\":true,\"id\":%d,\"prio\":%d,\"src\":%d,\"dst\":%d}",p->id,p->priorite,p->source,p->destination);
        else  printf("{\"ok\":false,\"message\":\"File vide\"}");
    } else if(strcmp(op,"stats")==0){
        printf("{\"taille\":%d,\"capacite\":%d,\"enfiles\":%d,\"defiles\":%d,\"perdus\":%d,\"taux_perte\":%.2f}",
               FA_api->taille_actuelle,FA_api->capacite_max,FA_api->total_enfiles,FA_api->total_defiles,FA_api->total_perdus,
               FA_api->total_enfiles>0?100.0f*FA_api->total_perdus/FA_api->total_enfiles:0.0f);
    } else if(strcmp(op,"list")==0){
        printf("{\"packets\":[");
        Paquet* p=FA_api->tete; int first=1;
        while(p){
            if(!first)printf(",");
            printf("{\"id\":%d,\"prio\":%d,\"src\":%d,\"dst\":%d,\"size\":%.1f}",p->id,p->priorite,p->source,p->destination,p->taille_Mo);
            first=0;
            p=p->suivant;
        }
        printf("],\"taille\":%d}",FA_api->taille_actuelle);
    } else if(strcmp(op,"simulate")==0){
        int n=15; 
        jint(j,"n",&n); 
        srand((unsigned)time(NULL));
        int nn=G_api?G_api->nb_noeuds:5;
        for(int i=0;i<n;i++){
            int p2=1+rand()%10,s2=rand()%nn,d2=rand()%nn;
            float sz2=0.1f+(float)(rand()%100)/10.0f;
            enqueue(FA_api,PKT_ID++,p2,sz2,s2,d2,(float)i);
        }
        printf("{\"ok\":true,\"simulated\":%d,\"in_queue\":%d,\"lost\":%d}",n,FA_api->taille_actuelle,FA_api->total_perdus);
    } else if(strcmp(op,"reset")==0){
        int cap=20; 
        jint(j,"cap",&cap);
        if(FA_api)liberer_file(FA_api);
        FA_api=creer_file(cap); 
        PKT_ID=1;
        printf("{\"ok\":true}");
    } else {
        printf("{\"ok\":false,\"message\":\"Operation inconnue\"}");
    }
}

/* ══════════════════════════════════════════════════════════════════════
   POINT D'ENTRÉE
   ══════════════════════════════════════════════════════════════════════ */
int run_api_mode(void) {
    /* Charger le graphe démo silencieusement */
    silent_load("data/reseau_demo.txt");
    FA_api = creer_file(20);

    /* Lire stdin */
    char input[8192]={0}; int c,i=0;
    while((c=getchar())!=EOF&&i<(int)sizeof(input)-1) input[i++]=(char)c;
    input[i]='\0';

    char cmd[64]=""; jstr(input,"cmd",cmd,sizeof(cmd));

    if      (!strcmp(cmd,"get_graph"))   print_graph();
    else if (!strcmp(cmd,"load_graph")) {
        char f[256]="data/reseau_demo.txt";
        jstr(input,"file",f,sizeof(f));
        silent_load(f);printf("{\"ok\":true,\"nb_noeuds\":%d}",G_api?G_api->nb_noeuds:0);
    }
    else if (!strcmp(cmd,"dijkstra"))    do_dijkstra(input);
    else if (!strcmp(cmd,"bellman"))     do_bellman(input);
    else if (!strcmp(cmd,"compare"))     do_compare(input);
    else if (!strcmp(cmd,"kpaths"))      do_kpaths(input);
    else if (!strcmp(cmd,"constrained")) do_constrained(input);
    else if (!strcmp(cmd,"dfs"))         do_dfs(input);
    else if (!strcmp(cmd,"bfs"))         do_bfs(input);
    else if (!strcmp(cmd,"cycles"))      { 
        int cy=G_api?detecter_cycles(G_api):0; 
        printf("{\"has_cycle\":%s}",cy?"true":"false"); 
    }
    else if (!strcmp(cmd,"articulation")){ 
        if(!G_api){
            printf("{\"points\":[],\"count\":0}");
        }else{
            ResultatSecurite r;
            memset(&r,0,sizeof(r));
            points_articulation(G_api,&r);
            printf("{\"points\":[");
            for(
                int i=0;
                i<r.nb_points_articulation;
                i++
            ){
                if(i)printf(",");
                printf("%d",r.points_articulation[i]);
            }
            printf("],\"count\":%d}",r.nb_points_articulation);
        }
    }
    else if (!strcmp(cmd,"bridges"))     { 
        if(!G_api){
            printf("{\"bridges\":[],\"count\":0}");
        }else{
            ResultatSecurite r;
            memset(&r,0,sizeof(r));
            ponts(G_api,&r);
            printf("{\"bridges\":[");
            for(
                int i=0;
                i<r.nb_ponts;
                i++
            ){
                if(i)printf(",");
                printf("{\"src\":%d,\"dst\":%d}",r.ponts_src[i],r.ponts_dest[i]);}printf("],\"count\":%d}",r.nb_ponts);
            }
        }
    else if (!strcmp(cmd,"components"))  { 
        if(!G_api){
            printf("{\"count\":0,\"mapping\":[]}");
        }else{
            ResultatSecurite r;
            memset(&r,0,sizeof(r));
            composantes_connexes(G_api,&r);
            printf("{\"count\":%d,\"mapping\":[",r.nb_composantes);
            for(
                int i=0;
                i<G_api->nb_noeuds;
                i++
            ){
                if(i)printf(",");
                printf("{\"node\":%d,\"comp\":%d}",i,r.composantes[i]);
            }printf("]}");}  }
    else if (!strcmp(cmd,"tarjan"))      do_tarjan();
    else if (!strcmp(cmd,"security"))    do_security_full();
    else if (!strcmp(cmd,"benchmark"))   do_benchmark();
    else if (!strcmp(cmd,"queue_ops"))   do_queue(input);
    else printf("{\"error\":\"Commande inconnue: %s\"}", cmd);

    if(G_api)  liberer_graphe(G_api);
    if(FA_api) liberer_file(FA_api);
    return 0;
}