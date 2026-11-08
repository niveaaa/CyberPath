#include "Traversal.h"

#include <chrono>
#include <queue>
#include <stdexcept>

namespace cyberpath {
namespace {

void validateSource(const Graph& graph, int sourceId) {
    if (!graph.device(sourceId).isActive) {
        throw std::invalid_argument("Traversal source device is inactive.");
    }
}

}  // namespace

BFSResult breadthFirstSearch(const Graph& graph, int sourceId) {
    validateSource(graph, sourceId);
    const auto start = std::chrono::steady_clock::now();

    BFSResult result;
    std::queue<int> pending;
    result.hopDistances.emplace(sourceId, 0);
    pending.push(sourceId);

    while (!pending.empty()) {
        const int currentId = pending.front();
        pending.pop();
        result.traversalOrder.push_back(currentId);

        for (const Edge& edge : graph.neighbors(currentId)) {
            ++result.metrics.edgesExamined;
            if (!edge.isActive || !graph.device(edge.destinationId).isActive ||
                result.hopDistances.find(edge.destinationId) !=
                    result.hopDistances.end()) {
                continue;
            }

            result.hopDistances.emplace(edge.destinationId,
                                        result.hopDistances.at(currentId) + 1);
            pending.push(edge.destinationId);
        }
    }

    result.reachableDevices = result.traversalOrder;
    result.metrics.verticesVisited = result.traversalOrder.size();
    result.metrics.executionTimeUs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start)
            .count());
    return result;
}

}  // namespace cyberpath
