#include "Traversal.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace {

int failures = 0;

cyberpath::Device makeDevice(int id) {
    cyberpath::Device device;
    device.id = id;
    device.name = "Device " + to_string(id);
    device.ipAddress = "192.0.2." + to_string(id);
    device.type = "workstation";
    return device;
}

void check(bool condition, const string& message) {
    if (!condition) {
        ++failures;
        cerr << "FAIL: " << message << '\n';
    }
}

void checkOrder(const vector<int>& actual,
                const vector<int>& expected,
                const string& label) {
    check(actual == expected, label);
}

void expectOutOfRange(const function<void()>& action,
                      const string& label) {
    try {
        action();
        check(false, label + " (did not throw)");
    } catch (const out_of_range&) {
    } catch (...) {
        check(false, label + " (threw wrong exception)");
    }
}

void expectInvalidArgument(const function<void()>& action,
                           const string& label) {
    try {
        action();
        check(false, label + " (did not throw)");
    } catch (const invalid_argument&) {
    } catch (...) {
        check(false, label + " (threw wrong exception)");
    }
}

cyberpath::Graph makeCyclicGraph() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(10));
    graph.addDevice(makeDevice(20));
    graph.addDevice(makeDevice(30));
    graph.addDevice(makeDevice(40));
    graph.addDevice(makeDevice(50));  // disconnected
    graph.addEdge(10, 20, 1.0);
    graph.addEdge(10, 30, 1.0);
    graph.addEdge(20, 30, 1.0);
    graph.addEdge(20, 40, 1.0);
    graph.addEdge(30, 40, 1.0);
    return graph;
}

void testBfsOrderReachabilityAndDistances() {
    const cyberpath::Graph graph = makeCyclicGraph();
    const cyberpath::BFSResult result =
        cyberpath::breadthFirstSearch(graph, 10);

    checkOrder(result.traversalOrder, {10, 20, 30, 40},
               "BFS visits breadth-first in ascending neighbor order");
    checkOrder(result.reachableDevices, result.traversalOrder,
               "BFS reachable devices match traversal order");
    check(result.hopDistances.size() == 4 && result.hopDistances.at(10) == 0 &&
              result.hopDistances.at(20) == 1 &&
              result.hopDistances.at(30) == 1 &&
              result.hopDistances.at(40) == 2,
          "BFS reports minimum hop distance for each reachable device");
    check(result.metrics.verticesVisited == 4,
          "BFS reports visited vertex count");
    check(result.metrics.edgesExamined == 10,
          "BFS counts each inspected adjacency entry");
}

void testDfsOrderAndMetrics() {
    const cyberpath::Graph graph = makeCyclicGraph();
    const cyberpath::DFSResult result =
        cyberpath::depthFirstSearch(graph, 10);

    checkOrder(result.traversalOrder, {10, 20, 30, 40},
               "iterative DFS follows recursive ascending-neighbor order");
    checkOrder(result.reachableDevices, result.traversalOrder,
               "DFS reachable devices match traversal order");
    check(result.metrics.verticesVisited == 4,
          "DFS reports visited vertex count");
    check(result.metrics.edgesExamined == 10,
          "DFS stops scanning a vertex after exploring all its neighbors");
}

void testSingleAndEmptyGraphs() {
    cyberpath::Graph single;
    single.addDevice(makeDevice(1000000));
    const auto bfs = cyberpath::breadthFirstSearch(single, 1000000);
    const auto dfs = cyberpath::depthFirstSearch(single, 1000000);
    checkOrder(bfs.traversalOrder, {1000000}, "BFS handles an isolated device");
    checkOrder(dfs.traversalOrder, {1000000}, "DFS handles an isolated device");
    check(bfs.hopDistances.at(1000000) == 0 && bfs.metrics.edgesExamined == 0,
          "single-device BFS distance and edge metrics are correct");
    check(dfs.metrics.edgesExamined == 0 && dfs.metrics.verticesVisited == 1,
          "single-device DFS metrics are correct");

    cyberpath::Graph empty;
    expectOutOfRange([&] { cyberpath::breadthFirstSearch(empty, 1); },
                     "BFS rejects a missing source");
    expectOutOfRange([&] { cyberpath::depthFirstSearch(empty, 1); },
                     "DFS rejects a missing source");
}

void testInactiveDevicesAndEdges() {
    cyberpath::Graph graph;
    graph.addDevice(makeDevice(1));
    graph.addDevice(makeDevice(2));
    graph.addDevice(makeDevice(3));
    graph.addDevice(makeDevice(4));
    graph.addEdge(1, 2, 1.0);
    graph.addEdge(2, 3, 1.0, false);
    graph.addEdge(3, 4, 1.0);

    const auto bfs = cyberpath::breadthFirstSearch(graph, 1);
    const auto dfs = cyberpath::depthFirstSearch(graph, 1);
    checkOrder(bfs.traversalOrder, {1, 2}, "BFS ignores inactive edges");
    checkOrder(dfs.traversalOrder, {1, 2}, "DFS ignores inactive edges");
    check(bfs.metrics.edgesExamined == 3 && dfs.metrics.edgesExamined == 3,
          "inactive edge adjacency entries are counted as examined");

    graph.setEdgeActive(2, 3, true);
    graph.setDeviceActive(3, false);
    checkOrder(cyberpath::breadthFirstSearch(graph, 1).traversalOrder, {1, 2},
               "BFS does not traverse into inactive devices");
    checkOrder(cyberpath::depthFirstSearch(graph, 1).traversalOrder, {1, 2},
               "DFS does not traverse into inactive devices");

    expectInvalidArgument(
        [&] { cyberpath::breadthFirstSearch(graph, 3); },
        "BFS rejects an inactive source");
    expectInvalidArgument(
        [&] { cyberpath::depthFirstSearch(graph, 3); },
        "DFS rejects an inactive source");
}

}  // namespace

int main() {
    testBfsOrderReachabilityAndDistances();
    testDfsOrderAndMetrics();
    testSingleAndEmptyGraphs();
    testInactiveDevicesAndEdges();

    if (failures != 0) {
        cerr << failures << " traversal check(s) failed.\n";
        return 1;
    }
    cout << "All traversal checks passed.\n";
    return 0;
}
