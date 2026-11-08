#include "Traversal.h"

#include <chrono>
#include <queue>
#include <stdexcept>

using namespace std;

namespace cyberpath {

BFSResult breadthFirstSearch(const Graph& graph, int sourceId) {
    if (!graph.device(sourceId).isActive) {
        throw invalid_argument("Traversal source device is inactive.");
    }

    auto start = chrono::steady_clock::now();
    BFSResult result;
    queue<int> pending;

    result.hopDistances[sourceId] = 0;
    pending.push(sourceId);

    while (!pending.empty()) {
        int currentId = pending.front();
        pending.pop();
        result.traversalOrder.push_back(currentId);

        for (const Edge& edge : graph.neighbors(currentId)) {
            result.metrics.edgesExamined++;

            if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
                continue;
            }

            if (result.hopDistances.find(edge.destinationId) == result.hopDistances.end()) {
                result.hopDistances[edge.destinationId] = result.hopDistances[currentId] + 1;
                pending.push(edge.destinationId);
            }
        }
    }

    result.reachableDevices = result.traversalOrder;
    result.metrics.verticesVisited = result.traversalOrder.size();
    result.metrics.executionTimeUs = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start).count();
    return result;
}

}  // namespace cyberpath
