#include "RiskAnalyzer.h"
#include "Simulation.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

cyberpath::Device makeDevice(int id, double vulnerability, double exposure,
                             double connectivity, double criticality,
                             bool isCritical = false) {
    cyberpath::Device device;
    device.id = id;
    device.name = "Device " + std::to_string(id);
    device.ipAddress = "192.0.2." + std::to_string(id);
    device.type = "server";
    device.vulnerabilityScore = vulnerability;
    device.exposureScore = exposure;
    device.connectivityScore = connectivity;
    device.criticalityScore = criticality;
    device.isCritical = isCritical;
    return device;
}

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void expectInvalidArgument(const std::function<void()>& action,
                           const std::string& message) {
    try {
        action();
        check(false, message + " (did not throw)");
    } catch (const std::invalid_argument&) {
    } catch (...) {
        check(false, message + " (threw wrong exception)");
    }
}

void expectOutOfRange(const std::function<void()>& action,
                      const std::string& message) {
    try {
        action();
        check(false, message + " (did not throw)");
    } catch (const std::out_of_range&) {
    } catch (...) {
        check(false, message + " (threw wrong exception)");
    }
}

cyberpath::Graph makeScenario() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(1, 10, 20, 30, 40));
    graph.addDevice(makeDevice(2, 20, 30, 40, 50));
    graph.addDevice(makeDevice(3, 30, 40, 50, 60, true));
    graph.addEdge(1, 2, 1.0);
    graph.addEdge(2, 3, 2.0);
    graph.addEdge(1, 3, 10.0);
    return graph;
}

void testFormulaCategoriesAndValidation() {
    const auto device = makeDevice(1, 100, 80, 60, 40, true);
    check(cyberpath::calculateRiskScore(device) == 78.0,
          "risk score uses exact 0.40/0.25/0.20/0.15 weighted formula");
    check(cyberpath::assessRisk(device).category == cyberpath::RiskCategory::High,
          "risk assessment assigns high category");
    check(cyberpath::riskCategoryName(cyberpath::RiskCategory::Moderate) ==
              std::string("Moderate"),
          "risk categories have readable names");
    check(cyberpath::classifyRisk(0.0) == cyberpath::RiskCategory::Low &&
              cyberpath::classifyRisk(33.999) == cyberpath::RiskCategory::Low,
          "low risk threshold covers scores below 34");
    check(cyberpath::classifyRisk(34.0) == cyberpath::RiskCategory::Moderate &&
              cyberpath::classifyRisk(66.999) ==
                  cyberpath::RiskCategory::Moderate,
          "moderate risk threshold covers scores from 34 to below 67");
    check(cyberpath::classifyRisk(67.0) == cyberpath::RiskCategory::High &&
              cyberpath::classifyRisk(100.0) == cyberpath::RiskCategory::High,
          "high risk threshold starts at 67");

    auto invalid = makeDevice(2, 0, 0, 0, 0);
    invalid.vulnerabilityScore = 101.0;
    expectInvalidArgument([&] { cyberpath::calculateRiskScore(invalid); },
                          "risk calculation rejects unnormalized scores");
    expectInvalidArgument([&] { cyberpath::classifyRisk(-0.1); },
                          "risk category rejects out-of-range score");
    auto flagOnly = makeDevice(3, 0, 0, 0, 0, true);
    check(cyberpath::calculateRiskScore(flagOnly) == 0.0,
          "critical-asset flag does not silently change numeric criticality score");
}

void testRiskRanking() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(3, 100, 100, 100, 100));
    graph.addDevice(makeDevice(2, 50, 50, 50, 50));
    graph.addDevice(makeDevice(1, 50, 50, 50, 50));
    const auto ranked = cyberpath::rankRisk(graph);
    check(ranked.size() == 3 && ranked[0].device.id == 3,
          "risk ranking sorts highest score first");
    check(ranked[1].device.id == 1 && ranked[2].device.id == 2,
          "equal risk scores use ascending ID tie-break");
    check(ranked[0].device.riskScore == 100.0 &&
              ranked[1].device.riskScore == 50.0,
          "ranked devices contain calculated scores");
    check(graph.device(3).riskScore == 0.0,
          "risk ranking does not mutate graph devices");
}

void testBaselineSimulationComparisonAndReset() {
    const cyberpath::Graph baseline = makeScenario();
    cyberpath::NetworkSimulation simulation(baseline);
    auto comparison = simulation.compare(1, 3);
    check(comparison.baseline.bfs.reachableDevices.size() == 3 &&
              comparison.simulated.bfs.reachableDevices.size() == 3,
          "initial simulation matches baseline reachability");
    check(comparison.baseline.dijkstra.totalCost == 3.0 &&
              comparison.simulated.dijkstra.path ==
                  std::vector<int>({1, 2, 3}),
          "initial simulation matches baseline minimum path");
    check(comparison.baseline.rankedRisks.size() == 3 &&
              comparison.simulated.rankedRisks.size() == 3,
          "comparison includes risk ranking in both states");

    simulation.setConnectionActive(1, 3, false);
    simulation.setDeviceActive(2, false);
    cyberpath::RiskAttributes changed{100.0, 100.0, 100.0, 100.0, true};
    simulation.setRiskAttributes(3, changed);
    comparison = simulation.compare(1, 3);
    check(comparison.baseline.dijkstra.totalCost == 3.0 &&
              comparison.baseline.bfs.reachableDevices.size() == 3,
          "simulation edits leave baseline topology unchanged");
    check(!comparison.simulated.dijkstra.reachable &&
              comparison.simulated.bfs.reachableDevices.size() == 1 &&
              comparison.simulated.dfs.reachableDevices.size() == 1,
          "disabled connection/device remove simulated reachability");
    check(comparison.simulated.rankedRisks.front().device.riskScore == 100.0 &&
              comparison.baseline.rankedRisks.front().device.id == 3 &&
              comparison.baseline.rankedRisks.front().device.riskScore == 41.0,
          "risk attribute edits affect simulation analysis only");
    check(simulation.baseline().device(2).isActive &&
              !simulation.simulated().device(2).isActive,
          "baseline and simulated device states are independent copies");

    simulation.setDeviceActive(2, true);
    simulation.setConnectionActive(1, 3, true);
    comparison = simulation.compare(1, 3);
    check(comparison.simulated.dijkstra.reachable &&
              comparison.simulated.dijkstra.totalCost == 3.0,
          "simulation supports re-enabling devices and connections");

    simulation.reset();
    comparison = simulation.compare(1, 3);
    check(comparison.simulated.dijkstra.totalCost == 3.0 &&
              comparison.simulated.rankedRisks.front().device.riskScore == 41.0 &&
              simulation.simulated().device(2).isActive,
          "reset restores all simulated topology and risk fields from baseline");
}

void testInactiveSourcesTargetsAndInvalidChanges() {
    cyberpath::NetworkSimulation simulation(makeScenario());
    simulation.setDeviceActive(3, false);
    auto comparison = simulation.compare(1, 3);
    check(!comparison.simulated.dijkstra.reachable &&
              comparison.simulated.bfs.reachableDevices.size() == 2,
          "disabled target is unreachable while traversal still reports active devices");

    simulation.setDeviceActive(1, false);
    comparison = simulation.compare(1, 3);
    check(comparison.simulated.bfs.traversalOrder.empty() &&
              !comparison.simulated.dijkstra.reachable,
          "disabled source produces empty traversal and unavailable path results");

    const cyberpath::RiskAttributes invalid{101.0, 0.0, 0.0, 0.0, false};
    expectInvalidArgument([&] { simulation.setRiskAttributes(2, invalid); },
                          "simulation rejects out-of-range risk changes");
    expectOutOfRange([&] { simulation.setDeviceActive(99, false); },
                     "simulation rejects unknown devices");
    expectOutOfRange(
        [&] { simulation.setConnectionActive(1, 99, false); },
        "simulation rejects unknown connections");
}

}  // namespace

int main() {
    testFormulaCategoriesAndValidation();
    testRiskRanking();
    testBaselineSimulationComparisonAndReset();
    testInactiveSourcesTargetsAndInvalidChanges();
    if (failures != 0) {
        std::cerr << failures << " risk/simulation check(s) failed.\n";
        return 1;
    }
    std::cout << "All risk and simulation checks passed.\n";
    return 0;
}
