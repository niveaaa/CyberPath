#include "Dijkstra.h"

#include "MinHeap.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

using namespace std;

namespace cyberpath {

DijkstraResult dijkstra(const Graph& graph, int sourceId, int targetId) {
    if (!graph.device(sourceId).isActive) {
        throw invalid_argument("Dijkstra source device is inactive.");
    }
    if (!graph.device(targetId).isActive) {
        throw invalid_argument("Dijkstra target device is inactive.");
    }

    auto start = chrono::steady_clock::now();

    DijkstraResult result;
    double infinity = numeric_limits<double>::infinity();
    map<int, double> distances;
    map<int, int> predecessors;
    for (const Device& device : graph.devices()) {
        distances.emplace(device.id, infinity);
    }
    distances.at(sourceId) = 0.0;

    MinHeap frontier;
    frontier.push({0.0, sourceId});
    ++result.metrics.heapPushes;

    while (!frontier.empty()) {
        HeapEntry current = frontier.pop();
        ++result.metrics.heapPops;
        if (current.priority != distances.at(current.deviceId)) {
            continue;  // A shorter route was queued after this entry.
        }

        ++result.metrics.verticesVisited;
        if (current.deviceId == targetId) {
            result.reachable = true;
            result.totalCost = current.priority;
            int pathDevice = targetId;
            result.path.push_back(pathDevice);
            while (pathDevice != sourceId) {
                const auto predecessor = predecessors.find(pathDevice);
                if (predecessor == predecessors.end() ||
                    result.path.size() > graph.deviceCount()) {
                    throw logic_error("Could not reconstruct Dijkstra path.");
                }
                pathDevice = predecessor->second;
                result.path.push_back(pathDevice);
            }
            reverse(result.path.begin(), result.path.end());
            break;
        }

        for (const Edge& edge : graph.neighbors(current.deviceId)) {
            ++result.metrics.edgesExamined;
            if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
                continue;
            }
            if (edge.cost < 0.0) {
                throw invalid_argument(
                    "Dijkstra requires non-negative edge costs.");
            }

            const double candidate = current.priority + edge.cost;
            if (!isfinite(candidate)) {
                throw overflow_error("Dijkstra path cost exceeds numeric range.");
            }
            if (candidate < distances.at(edge.destinationId)) {
                distances.at(edge.destinationId) = candidate;
                predecessors[edge.destinationId] = current.deviceId;
                frontier.push({candidate, edge.destinationId});
                ++result.metrics.heapPushes;
            }
        }
    }

    result.metrics.executionTimeUs = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start).count();
    return result;
}

}  // namespace cyberpath
