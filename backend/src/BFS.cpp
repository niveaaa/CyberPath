#include "Traversal.h"

#include <chrono>
#include <queue>
#include <stdexcept>

using namespace std;

namespace cyberpath {

BFSResult bfs(const Graph& graph, int sourceId, int targetId, bool stopAtTarget) {
    if (!graph.device(sourceId).isActive) {
        throw invalid_argument("Traversal source device is inactive.");
    }
    if (stopAtTarget) {
        graph.device(targetId);
    }

    auto start = chrono::steady_clock::now();
    BFSResult result;
    queue<int> waiting;

    result.distance[sourceId] = 0;
    waiting.push(sourceId);

    while (!waiting.empty()) {
        int current = waiting.front();
        waiting.pop();
        result.order.push_back(current);
        if (stopAtTarget && current == targetId) {
            result.targetFound = true;
            break;
        }

        for (const Edge& edge : graph.neighbors(current)) {
            result.metrics.checkedEdges++;

            if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
                continue;
            }

            if (result.distance.find(edge.destinationId) == result.distance.end()) {
                result.distance[edge.destinationId] = result.distance[current] + 1;
                waiting.push(edge.destinationId);
            }
        }
    }

    result.reachable = result.order;
    result.metrics.visited = result.order.size();
    result.metrics.timeUs = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start).count();
    return result;
}

BFSResult bfs(const Graph& graph, int sourceId) {
    return bfs(graph, sourceId, 0, false);
}

BFSResult bfs(const Graph& graph, int sourceId, int targetId) {
    return bfs(graph, sourceId, targetId, true);
}

}  // namespace cyberpath
