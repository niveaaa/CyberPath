#include "Traversal.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace cyberpath {
namespace {

void visitDevice(const Graph& graph, int deviceId, unordered_set<int>& visited,
                 DFSResult& result) {
    visited.insert(deviceId);
    result.traversalOrder.push_back(deviceId);

    for (const Edge& edge : graph.neighbors(deviceId)) {
        result.metrics.edgesExamined++;

        if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
            continue;
        }

        if (visited.find(edge.destinationId) == visited.end()) {
            visitDevice(graph, edge.destinationId, visited, result);
        }
    }
}

}  // namespace

DFSResult depthFirstSearch(const Graph& graph, int sourceId) {
    if (!graph.device(sourceId).isActive) {
        throw invalid_argument("Traversal source device is inactive.");
    }

    auto start = chrono::steady_clock::now();
    DFSResult result;
    unordered_set<int> visited;
    visitDevice(graph, sourceId, visited, result);

    result.reachableDevices = result.traversalOrder;
    result.metrics.verticesVisited = result.traversalOrder.size();
    result.metrics.executionTimeUs = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start).count();
    return result;
}

}  // namespace cyberpath
