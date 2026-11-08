#pragma once

#include "Graph.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

using namespace std;

namespace cyberpath {

struct TraversalMetrics {
    size_t verticesVisited = 0;
    size_t edgesExamined = 0;
    uint64_t executionTimeUs = 0;
};

struct BFSResult {
    vector<int> traversalOrder;
    vector<int> reachableDevices;
    map<int, size_t> hopDistances;
    TraversalMetrics metrics;
};

struct DFSResult {
    vector<int> traversalOrder;
    vector<int> reachableDevices;
    TraversalMetrics metrics;
};

// Counts every adjacency entry inspected, including inactive connections and
// connections leading to inactive devices. Such entries do not expand traversal.
BFSResult breadthFirstSearch(const Graph& graph, int sourceId);
DFSResult depthFirstSearch(const Graph& graph, int sourceId);

}  // namespace cyberpath
