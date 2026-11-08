#pragma once

#include "Dijkstra.h"
#include "RiskAnalyzer.h"
#include "Traversal.h"

#include <vector>

namespace cyberpath {

struct NetworkAnalysis {
    BFSResult bfs;
    DFSResult dfs;
    DijkstraResult dijkstra;
    std::vector<RiskAssessment> rankedRisks;
};

struct SimulationComparison {
    NetworkAnalysis baseline;
    NetworkAnalysis simulated;
};

// Owns an immutable baseline and a separate, editable simulation copy.
class NetworkSimulation {
public:
    explicit NetworkSimulation(const Graph& baseline);

    const Graph& baseline() const noexcept;
    const Graph& simulated() const noexcept;

    void setDeviceActive(int id, bool isActive);
    void setConnectionActive(int sourceId, int destinationId, bool isActive);
    void setRiskAttributes(int id, const RiskAttributes& attributes);
    void reset();

    SimulationComparison compare(int sourceId, int targetId) const;

private:
    Graph baseline_;
    Graph simulated_;

    static NetworkAnalysis analyze(const Graph& graph, int sourceId,
                                   int targetId);
};

}  // namespace cyberpath
