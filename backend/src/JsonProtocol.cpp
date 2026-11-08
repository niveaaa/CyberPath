#include "JsonProtocol.h"

#include "Dijkstra.h"
#include "Graph.h"
#include "RiskAnalyzer.h"
#include "Traversal.h"

#include <chrono>
#include <stdexcept>
#include <string>

using namespace std;

namespace cyberpath {
namespace {

const Json& required(const Json& object, const char* key, Json::value_t type) {
    if (!object.is_object() || !object.contains(key) || object.at(key).type() != type) {
        throw invalid_argument(string("Missing or invalid field: ") + key);
    }
    return object.at(key);
}

const Json& requiredInteger(const Json& object, const char* key) {
    if (!object.is_object() || !object.contains(key) ||
        !object.at(key).is_number_integer()) {
        throw invalid_argument(string("Missing or invalid integer field: ") + key);
    }
    return object.at(key);
}

const Json& requiredNumber(const Json& object, const char* key) {
    if (!object.is_object() || !object.contains(key) ||
        !object.at(key).is_number()) {
        throw invalid_argument(string("Missing or invalid number field: ") + key);
    }
    return object.at(key);
}

Graph parseNetwork(const Json& request) {
    const Json& network = required(request, "network", Json::value_t::object);
    const Json& devices = required(network, "devices", Json::value_t::array);
    const Json& edges = required(network, "edges", Json::value_t::array);
    Graph graph;
    for (const Json& item : devices) {
        requiredInteger(item, "id");
        required(item, "name", Json::value_t::string);
        required(item, "ip_address", Json::value_t::string);
        required(item, "type", Json::value_t::string);
        requiredNumber(item, "vulnerability_score");
        requiredNumber(item, "exposure_score");
        if (item.contains("is_active") && !item.at("is_active").is_boolean()) {
            throw invalid_argument("Device is_active flag must be a boolean.");
        }
        Device device;
        device.id = item.at("id").get<int>();
        device.name = item.at("name").get<string>();
        device.ipAddress = item.at("ip_address").get<string>();
        device.type = item.at("type").get<string>();
        device.vulnerabilityScore = item.at("vulnerability_score").get<double>();
        device.exposureScore = item.at("exposure_score").get<double>();
        device.isActive = item.value("is_active", true);
        graph.addDevice(device);
    }
    for (const Json& item : edges) {
        requiredInteger(item, "source_id");
        requiredInteger(item, "destination_id");
        requiredNumber(item, "cost");
        if (item.contains("is_active") && !item.at("is_active").is_boolean()) {
            throw invalid_argument("Edge is_active must be a boolean.");
        }
        graph.addEdge(item.at("source_id").get<int>(),
                      item.at("destination_id").get<int>(),
                      item.at("cost").get<double>(),
                      item.value("is_active", true));
    }
    return graph;
}

Json handleValidRequest(const Json& request, string& algorithm) {
    if (!request.is_object() || !request.contains("schema_version") ||
        !request.at("schema_version").is_number_integer() ||
        request.at("schema_version").get<int>() != 1) {
        throw invalid_argument("Unsupported schema_version; expected 1.");
    }
    algorithm = required(request, "algorithm", Json::value_t::string).get<string>();
    const Json& params = required(request, "parameters", Json::value_t::object);
    if (algorithm == "bfs" || algorithm == "dfs" || algorithm == "dijkstra") {
        requiredInteger(params, "source_id");
    }
    if (algorithm == "dijkstra") {
        requiredInteger(params, "target_id");
    } else if ((algorithm == "bfs" || algorithm == "dfs") &&
               params.contains("target_id")) {
        requiredInteger(params, "target_id");
    }
    const auto start = chrono::steady_clock::now();
    const Graph graph = parseNetwork(request);
    Json result;
    Json metrics = {{"devices", graph.deviceCount()}, {"edges", graph.edgeCount()}};
    Json steps = Json::array();

    if (algorithm == "bfs") {
        const int sourceId = params.at("source_id").get<int>();
        const auto value = params.contains("target_id")
            ? bfs(graph, sourceId, params.at("target_id").get<int>())
            : bfs(graph, sourceId);
        result = {{"traversal_order", value.order},
                  {"reachable_devices", value.reachable},
                  {"hop_distances", value.distance}};
        if (params.contains("target_id")) {
            result["target_reachable"] = value.targetFound;
            result["target_id"] = params.at("target_id");
        }
        metrics.update({{"vertices_visited", value.metrics.visited},
                        {"edges_examined", value.metrics.checkedEdges},
                        {"algorithm_time_us", value.metrics.timeUs}});
    } else if (algorithm == "dfs") {
        const int sourceId = params.at("source_id").get<int>();
        const auto value = params.contains("target_id")
            ? dfs(graph, sourceId, params.at("target_id").get<int>())
            : dfs(graph, sourceId);
        result = {{"traversal_order", value.order},
                  {"reachable_devices", value.reachable}};
        if (params.contains("target_id")) {
            result["target_reachable"] = value.targetFound;
            result["target_id"] = params.at("target_id");
        }
        metrics.update({{"vertices_visited", value.metrics.visited},
                        {"edges_examined", value.metrics.checkedEdges},
                        {"algorithm_time_us", value.metrics.timeUs}});
    } else if (algorithm == "dijkstra") {
        const auto value = dijkstra(graph, params.at("source_id").get<int>(),
                                    params.at("target_id").get<int>());
        result = {{"reachable", value.reachable}, {"path", value.path},
                  {"total_cost", value.reachable ? Json(value.cost) : Json(nullptr)}};
        metrics.update({{"vertices_visited", value.metrics.visited},
                        {"edges_examined", value.metrics.checkedEdges},
                        {"heap_pushes", value.metrics.heapPushes},
                        {"heap_pops", value.metrics.heapPops},
                        {"algorithm_time_us", value.metrics.timeUs}});
    } else if (algorithm == "risk") {
        Json ranked = Json::array();
        for (const auto& assessment : rankByRisk(graph)) {
            const Device& device = assessment.device;
            ranked.push_back({{"id", device.id}, {"name", device.name},
                              {"risk_score", device.riskScore},
                              {"category", riskCategoryName(assessment.category)}});
        }
        result = {{"ranked_devices", ranked}};
    } else {
        throw invalid_argument("Unsupported algorithm: " + algorithm);
    }

    const auto elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start).count();
    metrics["request_time_us"] = elapsed;
    return {{"schema_version", 1}, {"success", true}, {"algorithm", algorithm},
            {"result", result}, {"metrics", metrics}, {"steps", steps}};
}

}  // namespace

Json handleJsonRequest(const Json& request) {
    string algorithm;
    try {
        return handleValidRequest(request, algorithm);
    } catch (const exception& error) {
        return {{"schema_version", 1}, {"success", false},
                {"algorithm", algorithm.empty() ? Json(nullptr) : Json(algorithm)},
                {"error", {{"code", "INVALID_REQUEST"}, {"message", error.what()}}}};
    }
}

}  // namespace cyberpath
