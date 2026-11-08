#include "Traversal.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <set>

using namespace std;

namespace cyberpath {
namespace {

void visit(const Graph& graph, int deviceId, set<int>& visited,
                 DFSResult& result) {
    visited.insert(deviceId);
    result.order.push_back(deviceId);

    for (const Edge& edge : graph.neighbors(deviceId)) {
        result.metrics.checkedEdges++;

        if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
            continue;
        }

        if (visited.find(edge.destinationId) == visited.end()) {
            visit(graph, edge.destinationId, visited, result);
        }
    }
}

}  // namespace

DFSResult dfs(const Graph& graph, int sourceId) {
    if (!graph.device(sourceId).isActive) {
        throw invalid_argument("Traversal source device is inactive.");
    }

    auto start = chrono::steady_clock::now();
    DFSResult result;
    set<int> visited;
    visit(graph, sourceId, visited, result);

    result.reachable = result.order;
    result.metrics.visited = result.order.size();
    result.metrics.timeUs = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start).count();
    return result;
}

}  // namespace cyberpath
