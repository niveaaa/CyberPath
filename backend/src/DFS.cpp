#include "Traversal.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <set>

using namespace std;

namespace cyberpath {
namespace {

bool visit(const Graph& graph, int deviceId, int targetId, bool stopAtTarget,
           set<int>& visited, DFSResult& result) {
    visited.insert(deviceId);
    result.order.push_back(deviceId);
    if (stopAtTarget && deviceId == targetId) {
        result.targetFound = true;
        return true;
    }

    for (const Edge& edge : graph.neighbors(deviceId)) {
        result.metrics.checkedEdges++;

        if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
            continue;
        }

        if (visited.find(edge.destinationId) == visited.end()) {
            if (visit(graph, edge.destinationId, targetId, stopAtTarget,
                      visited, result)) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

DFSResult dfs(const Graph& graph, int sourceId, int targetId, bool stopAtTarget) {
    if (!graph.device(sourceId).isActive) {
        throw invalid_argument("Traversal source device is inactive.");
    }
    if (stopAtTarget) {
        graph.device(targetId);
    }

    auto start = chrono::steady_clock::now();
    DFSResult result;
    set<int> visited;
    visit(graph, sourceId, targetId, stopAtTarget, visited, result);

    result.reachable = result.order;
    result.metrics.visited = result.order.size();
    result.metrics.timeUs = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start).count();
    return result;
}

DFSResult dfs(const Graph& graph, int sourceId) {
    return dfs(graph, sourceId, 0, false);
}

DFSResult dfs(const Graph& graph, int sourceId, int targetId) {
    return dfs(graph, sourceId, targetId, true);
}

}  // namespace cyberpath
