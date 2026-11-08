#include "Traversal.h"

#include <chrono>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace cyberpath {
namespace {

void validateSource(const Graph& graph, int sourceId) {
    if (!graph.device(sourceId).isActive) {
        throw std::invalid_argument("Traversal source device is inactive.");
    }
}

struct Frame {
    int deviceId;
    std::size_t nextNeighbor;
};

}  // namespace

DFSResult depthFirstSearch(const Graph& graph, int sourceId) {
    validateSource(graph, sourceId);
    const auto start = std::chrono::steady_clock::now();

    DFSResult result;
    std::unordered_set<int> visited;
    visited.reserve(graph.deviceCount());
    std::vector<Frame> stack;
    visited.insert(sourceId);
    result.traversalOrder.push_back(sourceId);
    stack.push_back({sourceId, 0});

    while (!stack.empty()) {
        Frame& frame = stack.back();
        const auto& neighbors = graph.neighbors(frame.deviceId);
        if (frame.nextNeighbor == neighbors.size()) {
            stack.pop_back();
            continue;
        }

        const Edge& edge = neighbors[frame.nextNeighbor++];
        ++result.metrics.edgesExamined;
        if (!edge.isActive || !graph.device(edge.destinationId).isActive) {
            continue;
        }

        if (!visited.insert(edge.destinationId).second) {
            continue;
        }
        result.traversalOrder.push_back(edge.destinationId);
        stack.push_back({edge.destinationId, 0});
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
