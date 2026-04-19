# 🗺️ Travel Route & Cell Coverage Planner
> A DSA Mini Project in C — Dijkstra's shortest path + real-road map visualization

---

## Overview

This is a console-based travel planner for the **Maharashtra highway network** that finds the shortest route between any two cities using **Dijkstra's algorithm**, checks for mobile network dead zones along the way, and generates an interactive browser map with real road geometry.

---

## DSA Concepts Used

| Concept | How It's Used |
|---|---|
| **Graph** | Adjacency list (linked list of `Road` nodes) representing cities and highways |
| **Dijkstra's Algorithm** | Finds the shortest driving path between source and destination |
| **Stack** | Reconstructs the path in correct order from the `parent[]` array |
| **Structs** | `City` (name, coverage, GPS coords) and `Road` (destination, distance, next pointer) |

---

## Features

- Preloaded Maharashtra highway network (11 cities, 13 roads)
- Shortest path finder with distance in km
- Cell coverage check — flags dead zones on your route
- Add custom cities and roads at runtime
- Toggle coverage status of any city
- Auto-generates `route_map.html` and opens it in your browser
- Interactive map with:
  - Real road geometry via OSRM routing API (not straight lines)
  - Blue glowing route line for the shortest path
  - Grey roads for the full network
  - Clickable city markers showing coverage status
  - km distance labels on route segments

---

## Preloaded Map

```
Pune ── Lonavala ── Khopoli ── Mumbai
 |          └──────────────────┘
 ├── Nashik ── Igatpuri ── Mumbai
 |      └── Shirdi ── Aurangabad
 ├── Solapur ──────────── Aurangabad
 └── Satara ── Kolhapur
```

| City | Coverage |
|---|---|
| Pune | ✅ Covered |
| Lonavala | ✅ Covered |
| Khopoli | ❌ Dead Zone |
| Mumbai | ✅ Covered |
| Nashik | ✅ Covered |
| Igatpuri | ❌ Dead Zone |
| Shirdi | ✅ Covered |
| Aurangabad | ✅ Covered |
| Solapur | ✅ Covered |
| Satara | ✅ Covered |
| Kolhapur | ❌ Dead Zone |

---

## Requirements

- GCC (any version supporting C99 or later)
- A Linux/macOS/Windows system with a default browser
- Internet connection (only when viewing the map — for CartoDB tiles and OSRM routing)

No external C libraries needed. All map rendering is done via CDN in the generated HTML.

---

## Compile & Run

```bash
# Compile
gcc travel_planner.c -o travel_planner

# Run
./travel_planner
```

---

## Menu Options

```
1. View map       — show all cities/roads in terminal + open full network map
2. Plan trip      — enter source & destination IDs, get shortest route + map
3. Add a city     — add a new city with GPS coordinates and coverage status
4. Add a road     — connect two cities with a distance in km
5. Toggle coverage — flip a city between COVERED and DEAD ZONE
6. Exit
```

---

## How It Works

### 1. Graph Construction
Cities are stored in a `City[]` array. Roads are stored as a singly-linked adjacency list — each city holds a pointer to its first `Road` node, and each `Road` node points to the next. Roads are bidirectional (added in both directions).

### 2. Dijkstra's Algorithm
Runs on the adjacency list from the source city. Maintains `dist[]`, `visited[]`, and `parent[]` arrays. At each step, picks the unvisited city with the smallest known distance, then relaxes all its edges.

### 3. Path Reconstruction via Stack
After Dijkstra, the shortest path is reconstructed by walking `parent[]` backwards from destination to source. Since that gives the path in reverse, it's pushed onto a stack and popped to get the correct forward order.

### 4. Map Generation
`writeAndOpenMap()` writes a self-contained `route_map.html` file using:
- **Leaflet.js** for the interactive map
- **CartoDB tiles** (works from `file://` — no HTTP referer required)
- **OSRM API** (`router.project-osrm.org`) to fetch real driving road geometry for every road segment, replacing straight lines with actual highway curves

---

## Output Files

| File | Description |
|---|---|
| `travel_planner.c` | The only source file — compile this |
| `route_map.html` | Auto-generated on each map view/route plan — open in any browser |

> ⚠️ `route_map.html` is always **overwritten** each time you view the map or plan a trip. It is not a source file — do not edit it manually.

---

## Notes

- The JSON file (`cities_roads.json`) sometimes included with this project is **not used** by the program. It's a reference export of the hardcoded map data only.
- The program uses `xdg-open` (Linux), `open` (macOS), or `start` (Windows) to launch the map — whichever is available.
- OSRM road geometry requires an internet connection. If offline, the map falls back to straight lines between cities automatically.
