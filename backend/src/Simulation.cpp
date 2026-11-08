#include "Simulation.h"

using namespace std;

namespace cyberpath {

NetworkSimulation::NetworkSimulation(const Graph& baseline)
    : baseline_(baseline), simulated_(baseline) {}

const Graph& NetworkSimulation::baseline() const noexcept { return baseline_; }

const Graph& NetworkSimulation::simulated() const noexcept { return simulated_; }

void NetworkSimulation::setDeviceActive(int id, bool isActive) {
    simulated_.setDeviceActive(id, isActive);
}

void NetworkSimulation::setConnectionActive(int sourceId, int destinationId,
                                            bool isActive) {
    simulated_.setEdgeActive(sourceId, destinationId, isActive);
}

void NetworkSimulation::setRiskAttributes(
    int id, const RiskAttributes& attributes) {
    simulated_.setRiskAttributes(id, attributes);
}

void NetworkSimulation::reset() { simulated_ = baseline_; }

SimulationComparison NetworkSimulation::compare(int sourceId,
                                                int targetId) const {
    // Validate IDs in both states; simulation operations do not add/remove IDs.
    baseline_.device(sourceId);
    baseline_.device(targetId);
    return {analyze(baseline_, sourceId, targetId),
            analyze(simulated_, sourceId, targetId)};
}

NetworkAnalysis NetworkSimulation::analyze(const Graph& graph, int sourceId,
                                           int targetId) {
    NetworkAnalysis analysis;
    const bool sourceActive = graph.device(sourceId).isActive;
    const bool targetActive = graph.device(targetId).isActive;
    if (sourceActive) {
        analysis.bfs = breadthFirstSearch(graph, sourceId);
        analysis.dfs = depthFirstSearch(graph, sourceId);
        if (targetActive) {
            analysis.dijkstra = dijkstra(graph, sourceId, targetId);
        }
    }
    analysis.rankedRisks = rankRisk(graph);
    return analysis;
}

}  // namespace cyberpath
