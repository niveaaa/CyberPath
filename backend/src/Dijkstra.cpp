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
    map<int, int> previous;
    for (const Device& device : graph.devices()) {
        distances.emplace(device.id, infinity);
    }
    distances.at(sourceId) = 0.0;

    MinHeap heap;
    heap.push({0.0, sourceId});
    ++result.metrics.heapPushes;

    while (!heap.empty()) {
        HeapEntry current = heap.pop();
        ++result.metrics.heapPops;
        if (current.priority != distances.at(current.deviceId)) {
            continue;  // A shorter route was queued after this entry.
        }

        ++result.metrics.visited;
        if (current.deviceId == targetId) {
            result.reachable = true;
            result.cost = current.priority;
            int pathId = targetId;
            result.path.push_back(pathId);
            while (pathId != sourceId) {
                const auto predecessor = previous.find(pathId);
                if (predecessor == previous.end() ||
                    result.path.size() > graph.deviceCount()) {
                    throw logic_error("Could not reconstruct Dijkstra path.");
                }
                pathId = predecessor->second;
                result.path.push_back(pathId);
            }
            reverse(result.path.begin(), result.path.end());
            break;
        }

        for (const Edge& edge : graph.neighbors(current.deviceId)) {
            ++result.metrics.checkedEdges;
            if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
                continue;
            }
            if (edge.cost < 0.0) {
                throw invalid_argument(
                    "Dijkstra requires non-negative edge costs.");
            }

            const double newCost = current.priority + edge.cost;
            if (!isfinite(newCost)) {
                throw overflow_error("Dijkstra path cost exceeds numeric range.");
            }
            if (newCost < distances.at(edge.destinationId)) {
                distances.at(edge.destinationId) = newCost;
                previous[edge.destinationId] = current.deviceId;
                heap.push({newCost, edge.destinationId});
                ++result.metrics.heapPushes;
            }
        }
    }

    result.metrics.timeUs = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start).count();
    return result;
}

}  // namespace cyberpath
