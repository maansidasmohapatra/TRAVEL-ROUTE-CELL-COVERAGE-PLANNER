/*
    TRAVEL ROUTE & CELL COVERAGE PLANNER
    DSA Mini Project — C Language

    DSA Concepts:
      1. Graph        — adjacency list (linked list of Edge nodes)
      2. Dijkstra     — shortest path from A to B
      3. Stack        — reverse the path for correct display
      4. Struct       — City node, Road node

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX  20
#define INF  99999

/* Flush leftover input after every scanf.
   Without this, a leftover '\n' causes the
   menu to repeat the same option infinitely. */
#define FLUSH while(getchar() != '\n')

/* ── One city on the map ─────────────────── */
typedef struct {
    char name[40];
    int  covered;   /* 1 = cell tower present, 0 = dead zone */
} City;

/* ── One road in the adjacency list ─────── */
typedef struct Road {
    int         to;
    int         km;
    struct Road* next;
} Road;

City  city[MAX];
Road* adj[MAX];
int   total = 0;

/* ── Stack ───────────────────────────────── */
int stk[MAX], top = -1;
void push(int v)  { stk[++top] = v; }
int  pop()        { return stk[top--]; }
int  empty()      { return top == -1; }

/* ── Add city ─────────────────────────────── */
int addCity(char* name, int covered) {
    strcpy(city[total].name, name);
    city[total].covered = covered;
    adj[total] = NULL;
    return total++;
}

/* ── Add road (bidirectional) ─────────────── */
void addRoad(int a, int b, int km) {
    Road* r1 = malloc(sizeof(Road));
    r1->to = b; r1->km = km; r1->next = adj[a]; adj[a] = r1;

    Road* r2 = malloc(sizeof(Road));
    r2->to = a; r2->km = km; r2->next = adj[b]; adj[b] = r2;
}

/* ── Show map ─────────────────────────────── */
void showMap() {
    int i;
    printf("\n  ID  City                 Coverage     Roads\n");
    printf("  --  -------------------  -----------  -----\n");
    for (i = 0; i < total; i++) {
        printf("  %-2d  %-20s  %-11s  ",
               i, city[i].name,
               city[i].covered ? "[COVERED]" : "[DEAD ZONE]");
        Road* r = adj[i];
        while (r) {
            printf("%s(%dkm)", city[r->to].name, r->km);
            if (r->next) printf(", ");
            r = r->next;
        }
        printf("\n");
    }
}

/* ── Dijkstra ─────────────────────────────── */
void dijkstra(int src, int dst) {
    int dist[MAX], visited[MAX], parent[MAX], i;
    for (i = 0; i < total; i++) {
        dist[i] = INF; visited[i] = 0; parent[i] = -1;
    }
    dist[src] = 0;

    int step;
    for (step = 0; step < total - 1; step++) {
        int u = -1, best = INF;
        for (i = 0; i < total; i++)
            if (!visited[i] && dist[i] < best) { best = dist[i]; u = i; }
        if (u == -1) break;
        visited[u] = 1;

        Road* r = adj[u];
        while (r) {
            if (!visited[r->to] && dist[u] + r->km < dist[r->to]) {
                dist[r->to]   = dist[u] + r->km;
                parent[r->to] = u;
            }
            r = r->next;
        }
    }

    if (dist[dst] == INF) {
        printf("\n  No path found between %s and %s.\n",
               city[src].name, city[dst].name);
        return;
    }

    /* reconstruct via stack */
    int cur = dst;
    while (cur != -1) { push(cur); cur = parent[cur]; }
    int path[MAX], plen = 0;
    while (!empty()) path[plen++] = pop();

    printf("\n  Route (%d km): ", dist[dst]);
    for (i = 0; i < plen; i++) {
        if (i) printf(" --> ");
        printf("%s", city[path[i]].name);
    }

    printf("\n\n  Coverage check:\n");
    int dead = 0;
    for (i = 0; i < plen; i++) {
        if (city[path[i]].covered)
            printf("  [OK]  %s\n", city[path[i]].name);
        else {
            printf("  [!!]  %s  <-- DEAD ZONE\n", city[path[i]].name);
            dead++;
        }
    }
    printf("\n  Result: ");
    if (dead == 0)
        printf("Full coverage. Safe to travel with internet.\n");
    else
        printf("%d dead zone(s). You will lose internet there.\n", dead);
}

/* ── Plan trip ────────────────────────────── */
void planTrip() {
    int src, dst;
    showMap();
    printf("\n  From (ID): "); scanf("%d", &src); FLUSH;
    printf("  To   (ID): "); scanf("%d", &dst); FLUSH;
    if (src < 0 || src >= total || dst < 0 || dst >= total)
        printf("  Invalid IDs.\n");
    else if (src == dst)
        printf("  Same city.\n");
    else
        dijkstra(src, dst);
}

/* ── Add city menu ────────────────────────── */
void addCityMenu() {
    char name[40];
    int  cov;
    printf("  City name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    printf("  Has coverage? (1=Yes 0=No): ");
    scanf("%d", &cov); FLUSH;
    int id = addCity(name, cov);
    printf("  Added '%s' with ID %d.\n", name, id);
}

/* ── Add road menu ────────────────────────── */
void addRoadMenu() {
    int a, b, km;
    showMap();
    printf("\n  From ID: "); scanf("%d", &a); FLUSH;
    printf("  To   ID: "); scanf("%d", &b); FLUSH;
    printf("  Distance (km): "); scanf("%d", &km); FLUSH;
    if (a < 0 || a >= total || b < 0 || b >= total)
        printf("  Invalid IDs.\n");
    else {
        addRoad(a, b, km);
        printf("  Road added: %s <--> %s (%d km)\n",
               city[a].name, city[b].name, km);
    }
}

/* ── Toggle coverage ──────────────────────── */
void toggleCoverage() {
    int id;
    showMap();
    printf("\n  Enter ID to toggle: "); scanf("%d", &id); FLUSH;
    if (id < 0 || id >= total) { printf("  Invalid ID.\n"); return; }
    city[id].covered = !city[id].covered;
    printf("  '%s' is now: %s\n", city[id].name,
           city[id].covered ? "COVERED" : "DEAD ZONE");
}

/* ── Preloaded Maharashtra map ─────────────── */
void loadMap() {
    int pune       = addCity("Pune",       1);
    int lonavala   = addCity("Lonavala",   1);
    int khopoli    = addCity("Khopoli",    0);
    int mumbai     = addCity("Mumbai",     1);
    int nashik     = addCity("Nashik",     1);
    int igatpuri   = addCity("Igatpuri",   0);
    int shirdi     = addCity("Shirdi",     1);
    int aurangabad = addCity("Aurangabad", 1);
    int solapur    = addCity("Solapur",    1);
    int satara     = addCity("Satara",     1);
    int kolhapur   = addCity("Kolhapur",   0);

    addRoad(pune,     lonavala,    65);
    addRoad(lonavala, khopoli,     30);
    addRoad(khopoli,  mumbai,      55);
    addRoad(lonavala, mumbai,      83);
    addRoad(pune,     nashik,     210);
    addRoad(nashik,   igatpuri,    50);
    addRoad(igatpuri, mumbai,     120);
    addRoad(nashik,   shirdi,      90);
    addRoad(shirdi,   aurangabad, 115);
    addRoad(pune,     solapur,    250);
    addRoad(pune,     satara,     115);
    addRoad(satara,   kolhapur,   115);
    addRoad(solapur,  aurangabad, 265);
}

/* ── Free memory ──────────────────────────── */
void freeAll() {
    int i;
    for (i = 0; i < total; i++) {
        Road* r = adj[i];
        while (r) { Road* tmp = r->next; free(r); r = tmp; }
    }
}

/* ── Main ─────────────────────────────────── */
int main() {
    loadMap();
    int choice;
    printf("\n=== TRAVEL ROUTE & CELL COVERAGE PLANNER ===\n");
    printf("    Preloaded: Maharashtra highway network\n");

    do {
        printf("\n  1. View map\n");
        printf("  2. Plan trip\n");
        printf("  3. Add a city\n");
        printf("  4. Add a road\n");
        printf("  5. Toggle coverage\n");
        printf("  6. Exit\n");
        printf("  Choice: ");
        scanf("%d", &choice); FLUSH;

        switch (choice) {
            case 1: showMap();        break;
            case 2: planTrip();       break;
            case 3: addCityMenu();    break;
            case 4: addRoadMenu();    break;
            case 5: toggleCoverage(); break;
            case 6: printf("Bye!\n"); break;
            default: printf("  Invalid. Try 1-6.\n");
        }
    } while (choice != 6);

    freeAll();
    return 0;
}