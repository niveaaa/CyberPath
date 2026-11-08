#pragma once

#include "Graph.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace cyberpath {

struct DijkstraMetrics {
    std::size_t verticesVisited = 0;
    std::size_t edgesExamined = 0;
    std::size_t heapPushes = 0;
    std::size_t heapPops = 0;
    std::uint64_t executionTimeUs = 0;
};

struct DijkstraResult {
    bool reachable = false;
    std::vector<int> path;
    std::optional<double> totalCost;
    DijkstraMetrics metrics;
};

// Equal-cost paths keep the first predecessor discovered. The heap and graph
// neighbor ordering make that choice deterministic.
DijkstraResult dijkstra(const Graph& graph, int sourceId, int targetId);

}  // namespace cyberpath
