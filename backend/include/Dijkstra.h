#pragma once

#include "Graph.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

using namespace std;

namespace cyberpath {

struct DijkstraMetrics {
    size_t verticesVisited = 0;
    size_t edgesExamined = 0;
    size_t heapPushes = 0;
    size_t heapPops = 0;
    uint64_t executionTimeUs = 0;
};

struct DijkstraResult {
    bool reachable = false;
    vector<int> path;
    optional<double> totalCost;
    DijkstraMetrics metrics;
};

// Equal-cost paths keep the first predecessor discovered. The heap and graph
// neighbor ordering make that choice deterministic.
DijkstraResult dijkstra(const Graph& graph, int sourceId, int targetId);

}  // namespace cyberpath
