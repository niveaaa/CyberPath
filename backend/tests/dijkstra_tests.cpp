#include "Dijkstra.h"
#include "MinHeap.h"

#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

cyberpath::Device makeDevice(int id) {
    cyberpath::Device device;
    device.id = id;
    device.name = "Device " + std::to_string(id);
    device.ipAddress = "192.0.2." + std::to_string(id);
    device.type = "server";
    return device;
}

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void checkPath(const std::vector<int>& actual,
               const std::vector<int>& expected,
               const std::string& message) {
    check(actual == expected, message);
}

void expectUnderflow(const std::function<void()>& action,
                     const std::string& message) {
    try {
        action();
        check(false, message + " (did not throw)");
    } catch (const std::underflow_error&) {
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

void testHeapOrderingAndEmptyOperations() {
    cyberpath::MinHeap heap;
    check(heap.empty() && heap.size() == 0, "new heap is empty");
    expectUnderflow([&] { heap.top(); }, "top rejects empty heap");
    expectUnderflow([&] { heap.pop(); }, "pop rejects empty heap");

    heap.push({4.0, 4});
    heap.push({1.0, 3});
    heap.push({1.0, 1});
    heap.push({2.0, 2});
    heap.push({0.0, 8});
    check(heap.size() == 5 && heap.top().deviceId == 8,
          "heap reports size and minimum top");
    const std::vector<int> expectedIds{8, 1, 3, 2, 4};
    for (int expectedId : expectedIds) {
        check(heap.pop().deviceId == expectedId,
              "heap pops in priority and ID tie-break order");
    }
    check(heap.empty(), "heap is empty after all pops");
    heap.push({-1.0, 9});
    check(heap.top().deviceId == 9,
          "heap supports finite negative priorities independently of Dijkstra");
    expectInvalidArgument(
        [&] { heap.push({std::numeric_limits<double>::infinity(), 10}); },
        "heap rejects non-finite priorities");
}

void testShortestPathAndMetrics() {
    cyberpath::Graph graph;
    for (int id = 1; id <= 4; ++id) {
        graph.addDevice(makeDevice(id));
    }
    graph.addEdge(1, 2, 4.0);
    graph.addEdge(1, 3, 1.0);
    graph.addEdge(3, 2, 1.0);
    graph.addEdge(2, 4, 1.0);
    graph.addEdge(3, 4, 7.0);

    const auto result = cyberpath::dijkstra(graph, 1, 4);
    check(result.reachable, "Dijkstra finds a reachable target");
    checkPath(result.path, {1, 3, 2, 4}, "Dijkstra reconstructs minimum-cost path");
    check(result.totalCost && *result.totalCost == 3.0,
          "Dijkstra returns minimum total cost");
    check(result.metrics.verticesVisited == 4,
          "Dijkstra counts finalized vertices");
    check(result.metrics.edgesExamined == 8,
          "Dijkstra counts adjacency entries examined before target finalization");
    check(result.metrics.heapPushes == 6 && result.metrics.heapPops == 4,
          "Dijkstra reports heap pushes and pops");
}

void testEqualCostTieIsDeterministic() {
    cyberpath::Graph graph;
    for (int id = 1; id <= 4; ++id) {
        graph.addDevice(makeDevice(id));
    }
    graph.addEdge(1, 2, 1.0);
    graph.addEdge(1, 3, 1.0);
    graph.addEdge(2, 4, 1.0);
    graph.addEdge(3, 4, 1.0);

    const auto result = cyberpath::dijkstra(graph, 1, 4);
    checkPath(result.path, {1, 2, 4},
              "equal-cost alternatives use deterministic heap and neighbor order");
    check(result.totalCost && *result.totalCost == 2.0,
          "equal-cost alternative retains correct path cost");
}

void testStaleEntriesUnreachableAndSourceTarget() {
    cyberpath::Graph graph;
    for (int id = 1; id <= 4; ++id) {
        graph.addDevice(makeDevice(id));
    }
    graph.addEdge(1, 2, 10.0);
    graph.addEdge(1, 3, 1.0);
    graph.addEdge(3, 2, 1.0);

    const auto unreachable = cyberpath::dijkstra(graph, 1, 4);
    check(!unreachable.reachable && !unreachable.totalCost &&
              unreachable.path.empty(),
          "Dijkstra reports disconnected target without a path or finite cost");
    check(unreachable.metrics.heapPushes == 4 &&
              unreachable.metrics.heapPops == 4 &&
              unreachable.metrics.verticesVisited == 3,
          "Dijkstra counts and discards a stale heap entry");

    const auto same = cyberpath::dijkstra(graph, 2, 2);
    check(same.reachable && same.totalCost && *same.totalCost == 0.0,
          "source equal to target has zero cost");
    checkPath(same.path, {2}, "source-equals-target path contains one device");
    check(same.metrics.heapPushes == 1 && same.metrics.heapPops == 1,
          "source-equals-target heap counts are correct");
}

void testInactiveElementsAndInvalidEndpoints() {
    cyberpath::Graph graph;
    for (int id = 1; id <= 4; ++id) {
        graph.addDevice(makeDevice(id));
    }
    graph.addEdge(1, 2, 1.0);
    graph.addEdge(2, 3, 1.0);
    graph.addEdge(1, 4, 1.0, false);
    graph.setDeviceActive(3, false);

    const auto result = cyberpath::dijkstra(graph, 1, 4);
    check(!result.reachable,
          "Dijkstra ignores inactive edges and inactive devices");
    expectOutOfRange([&] { cyberpath::dijkstra(graph, 99, 1); },
                     "Dijkstra rejects missing source");
    expectOutOfRange([&] { cyberpath::dijkstra(graph, 1, 99); },
                     "Dijkstra rejects missing target");
    expectInvalidArgument([&] { cyberpath::dijkstra(graph, 1, 3); },
                          "Dijkstra rejects inactive target");
    graph.setDeviceActive(1, false);
    expectInvalidArgument([&] { cyberpath::dijkstra(graph, 1, 2); },
                          "Dijkstra rejects inactive source");
}

void testNegativeEdgeCostRejectedByGraph() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(1));
    graph.addDevice(makeDevice(2));
    expectInvalidArgument([&] { graph.addEdge(1, 2, -1.0); },
                          "graph rejects negative edge cost before Dijkstra");
}

}  // namespace

int main() {
    testHeapOrderingAndEmptyOperations();
    testShortestPathAndMetrics();
    testEqualCostTieIsDeterministic();
    testStaleEntriesUnreachableAndSourceTarget();
    testInactiveElementsAndInvalidEndpoints();
    testNegativeEdgeCostRejectedByGraph();

    if (failures != 0) {
        std::cerr << failures << " heap/Dijkstra check(s) failed.\n";
        return 1;
    }
    std::cout << "All heap and Dijkstra checks passed.\n";
    return 0;
}
