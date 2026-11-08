#pragma once

#include "Graph.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace cyberpath {

struct TraversalMetrics {
    std::size_t verticesVisited = 0;
    std::size_t edgesExamined = 0;
    std::uint64_t executionTimeUs = 0;
};

struct BFSResult {
    std::vector<int> traversalOrder;
    std::vector<int> reachableDevices;
    std::map<int, std::size_t> hopDistances;
    TraversalMetrics metrics;
};

struct DFSResult {
    std::vector<int> traversalOrder;
    std::vector<int> reachableDevices;
    TraversalMetrics metrics;
};

// Counts every adjacency entry inspected, including inactive connections and
// connections leading to inactive devices. Such entries do not expand traversal.
BFSResult breadthFirstSearch(const Graph& graph, int sourceId);
DFSResult depthFirstSearch(const Graph& graph, int sourceId);

}  // namespace cyberpath
