#include "RiskAnalyzer.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace {

int failures = 0;

cyberpath::Device makeDevice(int id, double vulnerability, double exposure) {
    cyberpath::Device device;
    device.id = id;
    device.name = "Device " + to_string(id);
    device.ipAddress = "192.0.2." + to_string(id);
    device.type = "server";
    device.vulnerabilityScore = vulnerability;
    device.exposureScore = exposure;
    return device;
}

void check(bool condition, const string& message) {
    if (!condition) {
        ++failures;
        cerr << "FAIL: " << message << '\n';
    }
}

void expectInvalidArgument(const function<void()>& action,
                           const string& message) {
    try {
        action();
        check(false, message + " (did not throw)");
    } catch (const invalid_argument&) {
    } catch (...) {
        check(false, message + " (threw wrong exception)");
    }
}

void testRiskScoreAndLevels() {
    const cyberpath::Device device = makeDevice(1, 100, 80);
    check(cyberpath::riskScore(device) == 90.0,
          "risk score is the average of vulnerability and exposure");
    check(cyberpath::assessRisk(device).category == cyberpath::RiskCategory::High,
          "risk assessment assigns high category");
    check(cyberpath::riskLevel(0.0) == cyberpath::RiskCategory::Low &&
              cyberpath::riskLevel(33.999) == cyberpath::RiskCategory::Low,
          "low risk covers scores below 34");
    check(cyberpath::riskLevel(34.0) == cyberpath::RiskCategory::Moderate &&
              cyberpath::riskLevel(66.999) == cyberpath::RiskCategory::Moderate,
          "moderate risk covers scores from 34 to below 67");
    check(cyberpath::riskLevel(67.0) == cyberpath::RiskCategory::High &&
              cyberpath::riskLevel(100.0) == cyberpath::RiskCategory::High,
          "high risk starts at 67");

    cyberpath::Device invalid = makeDevice(2, 0, 0);
    invalid.vulnerabilityScore = 101.0;
    expectInvalidArgument([&] { cyberpath::riskScore(invalid); },
                          "risk calculation rejects scores above 100");
    expectInvalidArgument([&] { cyberpath::riskLevel(-0.1); },
                          "risk level rejects scores below zero");
}

void testRiskRanking() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(3, 100, 100));
    graph.addDevice(makeDevice(2, 50, 50));
    graph.addDevice(makeDevice(1, 50, 50));
    const vector<cyberpath::RiskAssessment> ranked = cyberpath::rankByRisk(graph);

    check(ranked.size() == 3 && ranked[0].device.id == 3,
          "risk ranking puts the highest score first");
    check(ranked[1].device.id == 1 && ranked[2].device.id == 2,
          "equal risk scores use ascending device ID");
    check(ranked[0].device.riskScore == 100.0 &&
              ranked[1].device.riskScore == 50.0,
          "ranked devices contain calculated scores");
    check(graph.device(3).riskScore == 0.0,
          "risk ranking leaves the graph unchanged");
}

}  // namespace

int main() {
    testRiskScoreAndLevels();
    testRiskRanking();
    if (failures != 0) {
        cerr << failures << " risk check(s) failed.\n";
        return 1;
    }
    cout << "All risk checks passed.\n";
    return 0;
}
