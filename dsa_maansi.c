/*
    TRAVEL ROUTE & CELL COVERAGE PLANNER
    DSA Mini Project — C Language

    DSA Concepts:
      1. Graph        — adjacency list (linked list of Edge nodes)
      2. Dijkstra     — shortest path from A to B
      3. Stack        — reverse the path for correct display
      4. Struct       — City node, Road node

    Map output:
      - Writes a self-contained route_map.html using Leaflet.js (CDN)
      - Opens it with xdg-open (Linux) — no Python needed at all

    FIXES APPLIED:
      1. Tile provider switched to CartoDB (works from file://, no referer needed)
      2. OSRM API used to fetch real road geometry (replaces straight lines)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 20
#define INF 99999

#define FLUSH while (getchar() != '\n')

/* ── City ────────────────────────────────── */
typedef struct
{
    char  name[40];
    int   covered;
    double lat, lon;   /* GPS coords for the map */
} City;

/* ── Adjacency list node ─────────────────── */
typedef struct Road
{
    int to, km;
    struct Road *next;
} Road;

/* ── Flat road record (for HTML output) ──── */
typedef struct
{
    int a, b, km;
} RoadRecord;

City       city[MAX];
Road      *adj[MAX];
int        total = 0;

RoadRecord roads[MAX * MAX];
int        road_count = 0;

/* ── Stack ───────────────────────────────── */
int stk[MAX], top = -1;
void push(int v) { stk[++top] = v; }
int  pop()       { return stk[top--]; }
int  empty()     { return top == -1; }

/* ══════════════════════════════════════════
   HTML MAP WRITER
   Generates a complete Leaflet.js page and
   opens it in the default browser.
   path[]  = city indices on shortest route
             (NULL → show full network only)
   plen    = length of path array
   total_km= route distance (ignored if NULL)
   ══════════════════════════════════════════ */
void writeAndOpenMap(int *path, int plen, int total_km, int src, int dst)
{
    FILE *f = fopen("route_map.html", "w");
    if (!f) { printf("  [map] Cannot write route_map.html\n"); return; }

    /* ── mark which cities are on the path ── */
    int on_path[MAX] = {0};
    int i;
    for (i = 0; i < plen; i++) on_path[path[i]] = 1;

    /* ── mark which road segments are on the path ── */
    int pe_a[MAX], pe_b[MAX], pe_count = 0;
    for (i = 0; i < plen - 1; i++)
    {
        pe_a[pe_count] = path[i];
        pe_b[pe_count] = path[i + 1];
        pe_count++;
    }

    /* ════════════════════════════════════════
       HTML HEAD + LEAFLET CSS/JS
       ════════════════════════════════════════ */
    fprintf(f,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <meta charset='utf-8'/>\n"
        "  <title>Travel Route &amp; Cell Coverage Planner</title>\n"
        "  <meta name='viewport' content='width=device-width,initial-scale=1'/>\n"
        "  <link rel='stylesheet'"
        " href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>\n"
        "  <script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'>"
        "</script>\n"
        "  <style>\n"
        "    * { margin:0; padding:0; box-sizing:border-box; }\n"
        "    html, body, #map { width:100%%; height:100%%; }\n"
        "    /* Legend */\n"
        "    #legend {\n"
        "      position:fixed; bottom:20px; left:15px; z-index:9999;\n"
        "      background:#fff; border:1px solid #dadce0;\n"
        "      border-radius:10px; padding:12px 15px;\n"
        "      box-shadow:0 2px 10px rgba(0,0,0,0.15);\n"
        "      font-family:Arial,sans-serif; min-width:180px;\n"
        "    }\n"
        "    #legend h3 { color:#1a73e8; font-size:14px; margin-bottom:9px; }\n"
        "    .leg-row { display:flex; align-items:center;"
        "               gap:8px; margin:4px 0; font-size:12px; color:#444; }\n"
        "    .dot { width:11px; height:11px; border-radius:50%%;"
        "           display:inline-block; flex-shrink:0; }\n"
        "    #legend .footer { margin-top:9px; padding-top:8px;\n"
        "      border-top:1px solid #dadce0; font-size:10px; color:#888; }\n"
        "    /* km label on road */\n"
        "    .km-label { background:#fff; border:1px solid #1a73e8;\n"
        "      color:#1a73e8; font-size:11px; font-weight:bold;\n"
        "      padding:2px 7px; border-radius:20px; white-space:nowrap;\n"
        "      box-shadow:0 1px 4px rgba(0,0,0,.2); }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "<div id='map'></div>\n"
        "\n"
        "<!-- ═══ LEGEND ═══ -->\n"
        "<div id='legend'>\n"
    );

    /* legend title */
    if (plen > 0)
        fprintf(f, "  <h3>Route Planner</h3>\n");
    else
        fprintf(f, "  <h3>Network Map</h3>\n");

    /* legend rows */
    if (plen > 0)
    {
        fprintf(f,
            "  <div class='leg-row'>"
            "<span class='dot' style='background:#1a73e8'></span>"
            "From : %s</div>\n", city[src].name);
        fprintf(f,
            "  <div class='leg-row'>"
            "<span class='dot' style='background:#1a73e8'></span>"
            "To &nbsp;: %s</div>\n", city[dst].name);
        fprintf(f,
            "  <div class='leg-row'>"
            "<span class='dot' style='background:#555'></span>"
            "Dist : %d km</div>\n", total_km);
        fprintf(f,
            "  <div class='leg-row'>"
            "<span class='dot' style='background:#555'></span>"
            "Stops: %d</div>\n", plen);

        /* check dead zones on path */
        int has_dead = 0;
        for (i = 0; i < plen; i++) if (!city[path[i]].covered) has_dead = 1;

        if (has_dead)
        {
            fprintf(f,
                "  <div class='leg-row'>"
                "<span class='dot' style='background:#d93025'></span>"
                "Dead zones on route!</div>\n");
        }
        else
        {
            fprintf(f,
                "  <div class='leg-row'>"
                "<span class='dot' style='background:#34a853'></span>"
                "Full coverage &#x2714;</div>\n");
        }
        fprintf(f, "  <div class='footer'>Blue line = shortest path</div>\n");
    }
    else
    {
        /* count covered/dead */
        int covered_count = 0;
        for (i = 0; i < total; i++) if (city[i].covered) covered_count++;
        fprintf(f,
            "  <div class='leg-row'>"
            "<span class='dot' style='background:#1a73e8'></span>"
            "Cities : %d</div>\n", total);
        fprintf(f,
            "  <div class='leg-row'>"
            "<span class='dot' style='background:#34a853'></span>"
            "Covered: %d</div>\n", covered_count);
        fprintf(f,
            "  <div class='leg-row'>"
            "<span class='dot' style='background:#d93025'></span>"
            "Dead   : %d</div>\n", total - covered_count);
        fprintf(f,
            "  <div class='leg-row'>"
            "<span class='dot' style='background:#aaa'></span>"
            "Roads  : %d</div>\n", road_count);
        fprintf(f, "  <div class='footer'>Click any city for details</div>\n");
    }

    fprintf(f, "</div>\n\n");

    /* ════════════════════════════════════════
       JAVASCRIPT — build the map
       ════════════════════════════════════════ */
    fprintf(f, "<script>\n");

    /* ── city data array ── */
    fprintf(f, "var cities = [\n");
    for (i = 0; i < total; i++)
    {
        fprintf(f,
            "  {name:\"%s\", lat:%.4f, lon:%.4f,"
            " covered:%d, onPath:%d}%s\n",
            city[i].name, city[i].lat, city[i].lon,
            city[i].covered, on_path[i],
            i < total - 1 ? "," : "");
    }
    fprintf(f, "];\n\n");

    /* ── road data array ── */
    fprintf(f, "var roads = [\n");
    for (i = 0; i < road_count; i++)
    {
        int on_route = 0, j;
        for (j = 0; j < pe_count; j++)
        {
            if ((pe_a[j] == roads[i].a && pe_b[j] == roads[i].b) ||
                (pe_a[j] == roads[i].b && pe_b[j] == roads[i].a))
            {
                on_route = 1; break;
            }
        }
        fprintf(f,
            "  {a:%d, b:%d, km:%d, onRoute:%d}%s\n",
            roads[i].a, roads[i].b, roads[i].km, on_route,
            i < road_count - 1 ? "," : "");
    }
    fprintf(f, "];\n\n");

    /* ── path array (city indices in order) ── */
    fprintf(f, "var pathIndices = [");
    for (i = 0; i < plen; i++)
        fprintf(f, "%d%s", path[i], i < plen - 1 ? "," : "");
    fprintf(f, "];\n\n");

    /* ════════════════════════════════════════
       FIX 1: CartoDB tile layer
       Works from file:// — no HTTP referer required.
       Old OSM tile server blocked requests from local files.
       ════════════════════════════════════════ */
    fprintf(f,
        "// ── init map ──────────────────────────\n"
        "var map = L.map('map', {zoomControl:true});\n"
        "L.tileLayer('https://{s}.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}{r}.png',{\n"
        "  attribution:'&copy; <a href=\"https://carto.com/\">CARTO</a> &copy; OSM contributors',\n"
        "  subdomains:'abcd',\n"
        "  maxZoom:19\n"
        "}).addTo(map);\n\n"

        "// ── fit map to all cities ─────────────\n"
        "var lats = cities.map(function(c){return c.lat;});\n"
        "var lons = cities.map(function(c){return c.lon;});\n"
        "var minLat = Math.min.apply(null,lats) - 0.3;\n"
        "var maxLat = Math.max.apply(null,lats) + 0.3;\n"
        "var minLon = Math.min.apply(null,lons) - 0.3;\n"
        "var maxLon = Math.max.apply(null,lons) + 0.3;\n"
        "map.fitBounds([[minLat,minLon],[maxLat,maxLon]]);\n\n"
    );

    /* ════════════════════════════════════════
       OSRM real road geometry for ALL roads
       — grey network roads AND blue route.
       Every road segment is fetched from the
       free OSRM driving API so nothing looks
       like a straight spider-web line.
       ════════════════════════════════════════ */
    fprintf(f,
        "// ── fetch real road geometry for ALL roads via OSRM ──\n"
        "var fallbackLine = null;\n"

        /* dashed fallback for route only — visible while OSRM loads */
        "if (pathIndices.length >= 2) {\n"
        "  var fbCoords = pathIndices.map(function(i){\n"
        "    return [cities[i].lat, cities[i].lon];\n"
        "  });\n"
        "  fallbackLine = L.polyline(fbCoords,\n"
        "    {color:'#1a73e8', weight:3, opacity:0.30, dashArray:'7,9'}\n"
        "  ).addTo(map);\n"
        "}\n\n"

        "var allFetches = [];\n"
        "roads.forEach(function(r) {\n"
        "  var ca = cities[r.a], cb = cities[r.b];\n"
        "  var isRoute = r.onRoute;\n"
        "  var url = 'https://router.project-osrm.org/route/v1/driving/'\n"
        "    + ca.lon+','+ca.lat+';'\n"
        "    + cb.lon+','+cb.lat\n"
        "    + '?overview=full&geometries=geojson';\n"
        "  var p = fetch(url)\n"
        "    .then(function(res) { return res.json(); })\n"
        "    .then(function(data) {\n"
        "      if (!data.routes || !data.routes[0]) return;\n"
        "      var coords = data.routes[0].geometry.coordinates\n"
        "        .map(function(c) { return [c[1], c[0]]; });\n"
        "      if (isRoute) {\n"
        /* blue glowing route line */
        "        L.polyline(coords, {color:'#1a73e8', weight:14, opacity:0.10}).addTo(map);\n"
        "        L.polyline(coords, {color:'#1a73e8', weight:7,  opacity:0.20}).addTo(map);\n"
        "        L.polyline(coords, {color:'#1a73e8', weight:4,  opacity:0.95}).addTo(map);\n"
        "      } else {\n"
        /* grey network road */
        "        var line = L.polyline(coords,\n"
        "          {color:'#999999', weight:2.5, opacity:0.65}).addTo(map);\n"
        "        line.bindTooltip(\n"
        "          ca.name+' \u2014 '+cb.name+' ('+r.km+' km)',\n"
        "          {sticky:true});\n"
        "      }\n"
        "    })\n"
        "    .catch(function() {\n"
        /* fallback: draw straight line if OSRM fails for this segment */
        "      var color  = isRoute ? '#1a73e8' : '#aaaaaa';\n"
        "      var weight = isRoute ? 4 : 2;\n"
        "      L.polyline([[ca.lat,ca.lon],[cb.lat,cb.lon]],\n"
        "        {color:color, weight:weight, opacity:0.75}).addTo(map);\n"
        "    });\n"
        "  allFetches.push(p);\n"
        "});\n\n"

        /* remove dashed fallback once all real geometry is drawn */
        "Promise.all(allFetches).then(function() {\n"
        "  if (fallbackLine) { map.removeLayer(fallbackLine); fallbackLine = null; }\n"
        "}).catch(function() {});\n\n"
    );

    /* ── km labels at midpoints of route segments ── */
    fprintf(f,
        "// ── km labels on route ───────────────\n"
        "roads.forEach(function(r){\n"
        "  if (!r.onRoute) return;\n"
        "  var ca = cities[r.a], cb = cities[r.b];\n"
        "  var midLat = (ca.lat + cb.lat) / 2;\n"
        "  var midLon = (ca.lon + cb.lon) / 2;\n"
        "  var icon = L.divIcon({\n"
        "    className:'',\n"
        "    html:'<div class=\"km-label\">'+r.km+' km</div>',\n"
        "    iconSize:[70,22], iconAnchor:[35,11]\n"
        "  });\n"
        "  L.marker([midLat,midLon],{icon:icon}).addTo(map);\n"
        "});\n\n"
    );

    /* ── city markers ── */
    fprintf(f,
        "// ── city markers ─────────────────────\n"
        "cities.forEach(function(c, idx){\n"
        "  var fillColor, borderColor, radius;\n"
        "  if (c.onPath) {\n"
        "    fillColor   = c.covered ? '#1a73e8' : '#d93025';\n"
        "    borderColor = '#ffffff';\n"
        "    radius      = 10;\n"
        "  } else {\n"
        "    fillColor   = c.covered ? '#34a853' : '#8b0000';\n"
        "    borderColor = '#555555';\n"
        "    radius      = 7;\n"
        "  }\n"
        "  if (c.onPath) {\n"
        "    L.circleMarker([c.lat,c.lon],{\n"
        "      radius:radius+7, color:fillColor,\n"
        "      weight:1.5, fill:false, opacity:0.25\n"
        "    }).addTo(map);\n"
        "  }\n"
        "  var marker = L.circleMarker([c.lat,c.lon],{\n"
        "    radius:radius,\n"
        "    color:borderColor, weight: c.onPath ? 2.5 : 1.5,\n"
        "    fillColor:fillColor, fillOpacity:0.9\n"
        "  }).addTo(map);\n"
        "  var badgeColor = c.covered ? '#34a853' : '#d93025';\n"
        "  var badgeText  = c.covered ? 'COVERED' : 'DEAD ZONE';\n"
        "  var banner = c.onPath\n"
        "    ? '<div style=\"background:#e8f0fe;color:#1a73e8;font-size:10px;"\
              "font-weight:bold;padding:4px;text-align:center;"\
              "letter-spacing:1px;border-bottom:1px solid #dadce0;\">"\
              "ON SHORTEST PATH</div>'\n"
        "    : '';\n"
        "  var popup =\n"
        "    '<div style=\"font-family:Arial,sans-serif;\">'+banner+\n"
        "    '<div style=\"padding:10px 13px;\">'+\n"
        "    '<div style=\"font-size:15px;font-weight:bold;color:#202124;"\
              "margin-bottom:7px;\">'+c.name+'</div>'+\n"
        "    '<span style=\"background:'+badgeColor+'22;color:'+badgeColor+';"\
              "border:1px solid '+badgeColor+';padding:3px 10px;"\
              "border-radius:20px;font-size:11px;font-weight:bold;\">'\n"
        "    +badgeText+'</span></div></div>';\n"
        "  marker.bindPopup(popup, {maxWidth:220});\n"
        "  marker.bindTooltip(c.name, {sticky:false});\n"
        "  var labelIcon = L.divIcon({\n"
        "    className:'',\n"
        "    html:'<div style=\"font-family:Arial,sans-serif;font-size:11px;"\
              "font-weight:bold;color:#202124;white-space:nowrap;"\
              "text-shadow:1px 1px 0 #fff,-1px -1px 0 #fff,"\
              "1px -1px 0 #fff,-1px 1px 0 #fff;pointer-events:none;\">'\n"
        "         +c.name+'</div>',\n"
        "    iconSize:[130,16], iconAnchor:[65,16]\n"
        "  });\n"
        "  L.marker([c.lat + 0.02, c.lon],\n"
        "    {icon:labelIcon, interactive:false}).addTo(map);\n"
        "});\n"
    );

    fprintf(f, "</script>\n</body>\n</html>\n");
    fclose(f);

    printf("  [map] Saved -> route_map.html\n");
    system("xdg-open route_map.html 2>/dev/null || "
           "open route_map.html 2>/dev/null || "
           "start route_map.html");
}

/* ══════════════════════════════════════════
   REST OF THE PROGRAM (unchanged logic)
   ══════════════════════════════════════════ */

/* ── Add city ────────────────────────────── */
int addCity(char *name, int covered, double lat, double lon)
{
    strcpy(city[total].name, name);
    city[total].covered = covered;
    city[total].lat     = lat;
    city[total].lon     = lon;
    adj[total] = NULL;
    return total++;
}

/* ── Add road (bidirectional) ────────────── */
void addRoad(int a, int b, int km)
{
    Road *r1 = malloc(sizeof(Road));
    r1->to = b; r1->km = km; r1->next = adj[a]; adj[a] = r1;

    Road *r2 = malloc(sizeof(Road));
    r2->to = a; r2->km = km; r2->next = adj[b]; adj[b] = r2;

    roads[road_count].a  = a;
    roads[road_count].b  = b;
    roads[road_count].km = km;
    road_count++;
}

/* ── Show map (terminal table) ───────────── */
void showMap()
{
    int i;
    printf("\n  ID  City                 Coverage     Roads\n");
    printf("  --  -------------------  -----------  -----\n");
    for (i = 0; i < total; i++)
    {
        printf("  %-2d  %-20s  %-11s  ",
               i, city[i].name,
               city[i].covered ? "[COVERED]" : "[DEAD ZONE]");
        Road *r = adj[i];
        while (r)
        {
            printf("%s(%dkm)", city[r->to].name, r->km);
            if (r->next) printf(", ");
            r = r->next;
        }
        printf("\n");
    }
}

/* ── Dijkstra ────────────────────────────── */
void dijkstra(int src, int dst)
{
    int dist[MAX], visited[MAX], parent[MAX], i;
    for (i = 0; i < total; i++)
    { dist[i] = INF; visited[i] = 0; parent[i] = -1; }
    dist[src] = 0;

    int step;
    for (step = 0; step < total - 1; step++)
    {
        int u = -1, best = INF;
        for (i = 0; i < total; i++)
            if (!visited[i] && dist[i] < best) { best = dist[i]; u = i; }
        if (u == -1) break;
        visited[u] = 1;

        Road *r = adj[u];
        while (r)
        {
            if (!visited[r->to] && dist[u] + r->km < dist[r->to])
            {
                dist[r->to] = dist[u] + r->km;
                parent[r->to] = u;
            }
            r = r->next;
        }
    }

    if (dist[dst] == INF)
    {
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
    for (i = 0; i < plen; i++)
    { if (i) printf(" --> "); printf("%s", city[path[i]].name); }

    printf("\n\n  Coverage check:\n");
    int dead = 0;
    for (i = 0; i < plen; i++)
    {
        if (city[path[i]].covered)
            printf("  [OK]  %s\n", city[path[i]].name);
        else
        {
            printf("  [!!]  %s  <-- DEAD ZONE\n", city[path[i]].name);
            dead++;
        }
    }
    printf("\n  Result: ");
    if (dead == 0)
        printf("Full coverage. Safe to travel with internet.\n");
    else
        printf("%d dead zone(s). You will lose internet there.\n", dead);

    /* generate + open map */
    writeAndOpenMap(path, plen, dist[dst], src, dst);
}

/* ── Plan trip ───────────────────────────── */
void planTrip()
{
    int src, dst;
    showMap();
    printf("\n  From (ID): "); scanf("%d", &src); FLUSH;
    printf("  To   (ID): "); scanf("%d", &dst);  FLUSH;
    if (src < 0 || src >= total || dst < 0 || dst >= total)
        printf("  Invalid IDs.\n");
    else if (src == dst)
        printf("  Same city.\n");
    else
        dijkstra(src, dst);
}

/* ── View full network map ───────────────── */
void viewMap()
{
    showMap();
    int empty_path[1] = {0};
    writeAndOpenMap(empty_path, 0, 0, 0, 0);  /* plen=0 → network view */
}

/* ── Add city menu ───────────────────────── */
void addCityMenu()
{
    char name[40];
    int  cov;
    double lat, lon;
    printf("  City name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    printf("  Has coverage? (1=Yes 0=No): ");
    scanf("%d", &cov); FLUSH;
    printf("  Latitude  (e.g. 18.5204): ");
    scanf("%lf", &lat); FLUSH;
    printf("  Longitude (e.g. 73.8567): ");
    scanf("%lf", &lon); FLUSH;
    int id = addCity(name, cov, lat, lon);
    printf("  Added '%s' with ID %d.\n", name, id);
}

/* ── Add road menu ───────────────────────── */
void addRoadMenu()
{
    int a, b, km;
    showMap();
    printf("\n  From ID: "); scanf("%d", &a); FLUSH;
    printf("  To   ID: "); scanf("%d", &b); FLUSH;
    printf("  Distance (km): "); scanf("%d", &km); FLUSH;
    if (a < 0 || a >= total || b < 0 || b >= total)
        printf("  Invalid IDs.\n");
    else
    {
        addRoad(a, b, km);
        printf("  Road added: %s <--> %s (%d km)\n",
               city[a].name, city[b].name, km);
    }
}

/* ── Toggle coverage ─────────────────────── */
void toggleCoverage()
{
    int id;
    showMap();
    printf("\n  Enter ID to toggle: "); scanf("%d", &id); FLUSH;
    if (id < 0 || id >= total) { printf("  Invalid ID.\n"); return; }
    city[id].covered = !city[id].covered;
    printf("  '%s' is now: %s\n", city[id].name,
           city[id].covered ? "COVERED" : "DEAD ZONE");
}

/* ── Preloaded Maharashtra map ───────────── */
void loadMap()
{
    int pune       = addCity("Pune",       1, 18.5204, 73.8567);
    int lonavala   = addCity("Lonavala",   1, 18.7537, 73.4062);
    int khopoli    = addCity("Khopoli",    0, 18.7868, 73.3417);
    int mumbai     = addCity("Mumbai",     1, 19.0760, 72.8777);
    int nashik     = addCity("Nashik",     1, 19.9975, 73.7898);
    int igatpuri   = addCity("Igatpuri",   0, 19.6967, 73.5614);
    int shirdi     = addCity("Shirdi",     1, 19.7654, 74.4775);
    int aurangabad = addCity("Aurangabad", 1, 19.8762, 75.3433);
    int solapur    = addCity("Solapur",    1, 17.6868, 75.9064);
    int satara     = addCity("Satara",     1, 17.6805, 74.0183);
    int kolhapur   = addCity("Kolhapur",   0, 16.7050, 74.2433);

    addRoad(pune,       lonavala,    65);
    addRoad(lonavala,   khopoli,     30);
    addRoad(khopoli,    mumbai,      55);
    addRoad(lonavala,   mumbai,      83);
    addRoad(pune,       nashik,     210);
    addRoad(nashik,     igatpuri,    50);
    addRoad(igatpuri,   mumbai,     120);
    addRoad(nashik,     shirdi,      90);
    addRoad(shirdi,     aurangabad, 115);
    addRoad(pune,       solapur,    250);
    addRoad(pune,       satara,     115);
    addRoad(satara,     kolhapur,   115);
    addRoad(solapur,    aurangabad, 265);
}

/* ── Free memory ─────────────────────────── */
void freeAll()
{
    int i;
    for (i = 0; i < total; i++)
    {
        Road *r = adj[i];
        while (r) { Road *tmp = r->next; free(r); r = tmp; }
    }
}

/* ── Main ────────────────────────────────── */
int main()
{
    loadMap();
    int choice;
    printf("\n=== TRAVEL ROUTE & CELL COVERAGE PLANNER ===\n");
    printf("    Preloaded: Maharashtra highway network\n");

    do
    {
        printf("\n  1. View map\n");
        printf("  2. Plan trip\n");
        printf("  3. Add a city\n");
        printf("  4. Add a road\n");
        printf("  5. Toggle coverage\n");
        printf("  6. Exit\n");
        printf("  Choice: ");
        scanf("%d", &choice); FLUSH;

        switch (choice)
        {
        case 1: viewMap();        break;
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
