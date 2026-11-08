#pragma once

#include "Graph.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

using namespace std;

namespace cyberpath {

struct TraversalMetrics {
    size_t visited = 0;
    size_t checkedEdges = 0;
    uint64_t timeUs = 0;
};

struct BFSResult {
    vector<int> order;
    vector<int> reachable;
    map<int, size_t> distance;
    bool targetFound = false;
    TraversalMetrics metrics;
};

struct DFSResult {
    vector<int> order;
    vector<int> reachable;
    bool targetFound = false;
    TraversalMetrics metrics;
};

// Counts every adjacency entry inspected, including inactive connections and
// connections leading to inactive devices. Such entries do not expand traversal.
BFSResult bfs(const Graph& graph, int sourceId);
BFSResult bfs(const Graph& graph, int sourceId, int targetId);
DFSResult dfs(const Graph& graph, int sourceId);
DFSResult dfs(const Graph& graph, int sourceId, int targetId);

}  // namespace cyberpath
