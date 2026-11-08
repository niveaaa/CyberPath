#include "Graph.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

cyberpath::Device makeDevice(int id, const std::string& name,
                             const std::string& ip) {
    cyberpath::Device device;
    device.id = id;
    device.name = name;
    device.ipAddress = ip;
    device.type = "workstation";
    device.vulnerabilityScore = 25.0;
    device.exposureScore = 35.0;
    device.connectivityScore = 50.0;
    device.criticalityScore = 20.0;
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

void testEmptyGraphAndDeviceValidation() {
    cyberpath::Graph graph;
    check(graph.deviceCount() == 0 && graph.edgeCount() == 0,
          "new graph is empty");
    check(!graph.removeDevice(1), "removing unknown device returns false");
    expectOutOfRange([&] { graph.device(1); }, "unknown device lookup rejected");
    expectOutOfRange([&] { graph.neighbors(1); }, "unknown adjacency lookup rejected");

    graph.addDevice(makeDevice(1, "Gateway", "192.0.2.1"));
    check(graph.containsDevice(1), "added device is present");
    check(graph.device(1).name == "Gateway", "device fields are retained");
    check(graph.deviceIdByName("Gateway") == std::optional<int>{1} &&
              graph.deviceIdByIpAddress("192.0.2.1") == std::optional<int>{1},
          "custom hash indexes find devices by name and IP address");
    check(!graph.deviceIdByName("Missing") &&
              !graph.deviceIdByIpAddress("192.0.2.99"),
          "custom hash indexes report missing names and IP addresses");
    expectInvalidArgument(
        [&] { graph.addDevice(makeDevice(1, "Duplicate ID", "192.0.2.2")); },
        "duplicate ID rejected");
    expectInvalidArgument(
        [&] { graph.addDevice(makeDevice(2, "Gateway", "192.0.2.2")); },
        "duplicate name rejected");
    expectInvalidArgument(
        [&] { graph.addDevice(makeDevice(2, "Node", "192.0.2.1")); },
        "duplicate IP rejected");

    auto invalid = makeDevice(-1, "Negative", "192.0.2.3");
    expectInvalidArgument([&] { graph.addDevice(invalid); },
                          "negative ID rejected");
    invalid = makeDevice(2, "Bad score", "192.0.2.3");
    invalid.exposureScore = 100.1;
    expectInvalidArgument([&] { graph.addDevice(invalid); },
                          "score above 100 rejected");
    invalid = makeDevice(2, "Bad score", "192.0.2.3");
    invalid.vulnerabilityScore = std::nan("");
    expectInvalidArgument([&] { graph.addDevice(invalid); },
                          "non-finite component rejected");
    invalid = makeDevice(2, "Missing type", "192.0.2.3");
    invalid.type.clear();
    expectInvalidArgument([&] { graph.addDevice(invalid); },
                          "empty required field rejected");
}

void testUndirectedAdjacencyAndOrdering() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(1, "One", "192.0.2.1"));
    graph.addDevice(makeDevice(2, "Two", "192.0.2.2"));
    graph.addDevice(makeDevice(3, "Three", "192.0.2.3"));
    graph.addDevice(makeDevice(4, "Four", "192.0.2.4"));
    graph.addEdge(1, 3, 2.5);
    graph.addEdge(1, 2, 1.25);

    check(graph.edgeCount() == 2, "logical edge count counts each undirected edge once");
    check(graph.neighbors(1).size() == 2, "source has both adjacency entries");
    check(graph.neighbors(1)[0].destinationId == 2 &&
              graph.neighbors(1)[1].destinationId == 3,
          "neighbors are deterministic in ascending ID order");
    check(graph.neighbors(1)[0].sourceId == 1 &&
              graph.neighbors(1)[0].cost == 1.25,
          "forward edge retains source, destination, and cost");
    check(graph.neighbors(2).size() == 1 &&
              graph.neighbors(2)[0].sourceId == 2 &&
              graph.neighbors(2)[0].destinationId == 1,
          "reverse adjacency entry is consistent");
    check(graph.edges().size() == 2 && graph.edges()[0].sourceId == 1 &&
              graph.edges()[0].destinationId == 2,
          "edges returns each logical edge once in stable order");
    check(graph.containsEdge(3, 1) && graph.containsEdge(1, 3),
          "edge lookup is orientation independent");

    expectInvalidArgument([&] { graph.addEdge(1, 2, 1.0); },
                          "duplicate undirected edge rejected in either orientation");
    expectInvalidArgument([&] { graph.addEdge(1, 4, -0.1); },
                          "negative edge cost rejected");
    expectInvalidArgument([&] { graph.addEdge(1, 4, INFINITY); },
                          "infinite edge cost rejected");
    expectInvalidArgument([&] { graph.addEdge(1, 1, 1.0); },
                          "self connection rejected");
    expectInvalidArgument([&] { graph.addEdge(1, 99, 1.0); },
                          "unknown endpoint rejected");
}

void testActivationAndRemoval() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(1, "One", "192.0.2.1"));
    graph.addDevice(makeDevice(2, "Two", "192.0.2.2"));
    graph.addDevice(makeDevice(3, "Three", "192.0.2.3"));
    graph.addEdge(1, 2, 4.0);
    graph.addEdge(2, 3, 5.0, false);

    graph.setDeviceActive(2, false);
    check(!graph.device(2).isActive, "device active state can be disabled");
    graph.setDeviceActive(2, true);
    check(graph.device(2).isActive, "device active state can be re-enabled");
    graph.setEdgeActive(2, 3, true);
    check(graph.neighbors(2)[1].isActive && graph.neighbors(3)[0].isActive,
          "edge active state is synchronized in both directions");
    graph.setEdgeActive(3, 2, false);
    check(!graph.neighbors(2)[1].isActive && !graph.neighbors(3)[0].isActive,
          "edge active state setter accepts reversed endpoint order");
    expectOutOfRange([&] { graph.setDeviceActive(99, false); },
                     "activation of unknown device rejected");
    expectOutOfRange([&] { graph.setEdgeActive(1, 3, false); },
                     "activation of unknown edge rejected");

    check(graph.removeEdge(3, 2), "edge removal accepts reversed endpoints");
    check(graph.edgeCount() == 1 && graph.neighbors(2).size() == 1 &&
              graph.neighbors(3).empty(),
          "edge removal updates both lists and count");
    check(!graph.removeEdge(2, 3), "removing absent edge returns false");
    check(graph.removeDevice(2), "existing device can be removed");
    check(graph.deviceCount() == 2 && graph.edgeCount() == 0 &&
              graph.neighbors(1).empty() && graph.neighbors(3).empty(),
          "device removal cascades to all incident edges");
    check(!graph.containsDevice(2) && !graph.removeDevice(2),
          "removed device is absent and repeated removal is harmless");
    check(!graph.deviceIdByName("Two") &&
              !graph.deviceIdByIpAddress("192.0.2.2"),
          "device removal deletes its name and IP index entries");
    graph.addDevice(makeDevice(2, "Two", "192.0.2.2"));
    check(graph.deviceIdByName("Two") == std::optional<int>{2},
          "removed name and IP can be reused by a later device");
}

void testSortedDeviceSnapshot() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(8, "Eight", "192.0.2.8"));
    graph.addDevice(makeDevice(3, "Three", "192.0.2.3"));
    const auto devices = graph.devices();
    check(devices.size() == 2 && devices[0].id == 3 && devices[1].id == 8,
          "device snapshot is ordered by ID");
}

}  // namespace

int main() {
    testEmptyGraphAndDeviceValidation();
    testUndirectedAdjacencyAndOrdering();
    testActivationAndRemoval();
    testSortedDeviceSnapshot();

    if (failures != 0) {
        std::cerr << failures << " graph check(s) failed.\n";
        return 1;
    }
    std::cout << "All graph checks passed.\n";
    return 0;
}
