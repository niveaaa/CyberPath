#pragma once

#include "Graph.h"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace std;

namespace cyberpath {

struct DijkstraMetrics {
    size_t visited = 0;
    size_t checkedEdges = 0;
    size_t heapPushes = 0;
    size_t heapPops = 0;
    uint64_t timeUs = 0;
};

struct DijkstraResult {
    bool reachable = false;
    vector<int> path;
    double cost = 0.0;
    DijkstraMetrics metrics;
};

// Equal-cost paths keep the first predecessor discovered. The heap and graph
// neighbor ordering make that choice deterministic.
DijkstraResult dijkstra(const Graph& graph, int sourceId, int targetId);

}  // namespace cyberpath
