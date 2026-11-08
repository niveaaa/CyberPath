#include "Dijkstra.h"

#include "MinHeap.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace cyberpath {
namespace {

void validateEndpoint(const Graph& graph, int id, const char* role) {
    if (!graph.device(id).isActive) {
        throw std::invalid_argument(std::string("Dijkstra ") + role +
                                    " device is inactive.");
    }
}

}  // namespace

DijkstraResult dijkstra(const Graph& graph, int sourceId, int targetId) {
    validateEndpoint(graph, sourceId, "source");
    validateEndpoint(graph, targetId, "target");
    const auto start = std::chrono::steady_clock::now();

    DijkstraResult result;
    const double infinity = std::numeric_limits<double>::infinity();
    std::map<int, double> distances;
    std::map<int, int> predecessors;
    for (const Device& device : graph.devices()) {
        distances.emplace(device.id, infinity);
    }
    distances.at(sourceId) = 0.0;

    MinHeap frontier;
    frontier.push({0.0, sourceId});
    ++result.metrics.heapPushes;

    while (!frontier.empty()) {
        const HeapEntry current = frontier.pop();
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
                    throw std::logic_error("Could not reconstruct Dijkstra path.");
                }
                pathDevice = predecessor->second;
                result.path.push_back(pathDevice);
            }
            std::reverse(result.path.begin(), result.path.end());
            break;
        }

        for (const Edge& edge : graph.neighbors(current.deviceId)) {
            ++result.metrics.edgesExamined;
            if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
                continue;
            }
            if (edge.cost < 0.0) {
                throw std::invalid_argument(
                    "Dijkstra requires non-negative edge costs.");
            }

            const double candidate = current.priority + edge.cost;
            if (!std::isfinite(candidate)) {
                throw std::overflow_error("Dijkstra path cost exceeds numeric range.");
            }
            if (candidate < distances.at(edge.destinationId)) {
                distances.at(edge.destinationId) = candidate;
                predecessors[edge.destinationId] = current.deviceId;
                frontier.push({candidate, edge.destinationId});
                ++result.metrics.heapPushes;
            }
        }
    }

    result.metrics.executionTimeUs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start)
            .count());
    return result;
}

}  // namespace cyberpath
