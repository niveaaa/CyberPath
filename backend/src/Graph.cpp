#include "Graph.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

using namespace std;

namespace cyberpath {

void Graph::addDevice(const Device& device) {
    validateDevice(device);
    if (devices_.find(device.id) != devices_.end()) {
        throw invalid_argument("Device ID already exists.");
    }
    if (deviceIdsByName_.contains(device.name)) {
        throw invalid_argument("Device name already exists.");
    }
    if (deviceIdsByIpAddress_.contains(device.ipAddress)) {
        throw invalid_argument("Device IP address already exists.");
    }

    devices_.emplace(device.id, device);
    adjacency_.emplace(device.id, vector<Edge>{});
    deviceIdsByName_.insert(device.name, device.id);
    deviceIdsByIpAddress_.insert(device.ipAddress, device.id);
}

bool Graph::removeDevice(int id) {
    const auto found = devices_.find(id);
    if (found == devices_.end()) {
        return false;
    }

    vector<Edge> incident = adjacency_.at(id);
    for (const Edge& edge : incident) {
        removeEdge(id, edge.destinationId);
    }
    deviceIdsByName_.erase(found->second.name);
    deviceIdsByIpAddress_.erase(found->second.ipAddress);
    adjacency_.erase(id);
    devices_.erase(found);
    return true;
}

bool Graph::containsDevice(int id) const noexcept {
    return devices_.find(id) != devices_.end();
}

const Device& Graph::device(int id) const {
    const auto found = devices_.find(id);
    if (found == devices_.end()) {
        throw out_of_range("Device ID does not exist.");
    }
    return found->second;
}

int Graph::findDeviceByName(const string& name) const {
    const int* id = deviceIdsByName_.find(name);
    if (id == nullptr) {
        return -1;
    }
    return *id;
}

int Graph::findDeviceByIp(const string& ipAddress) const {
    const int* id = deviceIdsByIpAddress_.find(ipAddress);
    if (id == nullptr) {
        return -1;
    }
    return *id;
}

vector<Device> Graph::devices() const {
    vector<Device> result;
    result.reserve(devices_.size());
    for (const auto& entry : devices_) {
        result.push_back(entry.second);
    }
    return result;
}

size_t Graph::deviceCount() const noexcept { return devices_.size(); }

void Graph::addEdge(int sourceId, int destinationId, double cost,
                    bool isActive) {
    if (!containsDevice(sourceId) || !containsDevice(destinationId)) {
        throw invalid_argument("Both edge endpoints must exist.");
    }
    if (sourceId == destinationId) {
        throw invalid_argument("Self-connections are not supported.");
    }
    if (!isfinite(cost) || cost < 0.0) {
        throw invalid_argument("Edge cost must be finite and non-negative.");
    }
    if (containsEdge(sourceId, destinationId)) {
        throw invalid_argument("Connection already exists.");
    }

    vector<Edge>& sourceNeighbors = adjacency_.at(sourceId);
    auto sourcePosition = sourceNeighbors.begin();
    while (sourcePosition != sourceNeighbors.end() &&
           sourcePosition->destinationId < destinationId) {
        ++sourcePosition;
    }
    sourceNeighbors.insert(sourcePosition,
                           Edge{sourceId, destinationId, cost, isActive});

    vector<Edge>& destinationNeighbors = adjacency_.at(destinationId);
    auto destinationPosition = destinationNeighbors.begin();
    while (destinationPosition != destinationNeighbors.end() &&
           destinationPosition->destinationId < sourceId) {
        ++destinationPosition;
    }
    destinationNeighbors.insert(
        destinationPosition, Edge{destinationId, sourceId, cost, isActive});
    ++edgeCount_;
}

bool Graph::removeEdge(int sourceId, int destinationId) {
    if (!containsDevice(sourceId) || !containsDevice(destinationId) ||
        !containsEdge(sourceId, destinationId)) {
        return false;
    }
    vector<Edge>& sourceNeighbors = adjacency_.at(sourceId);
    for (auto edge = sourceNeighbors.begin(); edge != sourceNeighbors.end(); ++edge) {
        if (edge->destinationId == destinationId) {
            sourceNeighbors.erase(edge);
            break;
        }
    }
    vector<Edge>& destinationNeighbors = adjacency_.at(destinationId);
    for (auto edge = destinationNeighbors.begin(); edge != destinationNeighbors.end(); ++edge) {
        if (edge->destinationId == sourceId) {
            destinationNeighbors.erase(edge);
            break;
        }
    }
    --edgeCount_;
    return true;
}

bool Graph::containsEdge(int sourceId, int destinationId) const {
    const auto source = adjacency_.find(sourceId);
    if (source == adjacency_.end()) {
        return false;
    }
    for (const Edge& edge : source->second) {
        if (edge.destinationId == destinationId) {
            return true;
        }
    }
    return false;
}

void Graph::setEdgeActive(int sourceId, int destinationId, bool isActive) {
    if (!containsEdge(sourceId, destinationId)) {
        throw out_of_range("Connection does not exist.");
    }
    for (Edge& edge : adjacency_.at(sourceId)) {
        if (edge.destinationId == destinationId) {
            edge.isActive = isActive;
        }
    }
    for (Edge& edge : adjacency_.at(destinationId)) {
        if (edge.destinationId == sourceId) {
            edge.isActive = isActive;
        }
    }
}

const vector<Edge>& Graph::neighbors(int id) const {
    const auto found = adjacency_.find(id);
    if (found == adjacency_.end()) {
        throw out_of_range("Device ID does not exist.");
    }
    return found->second;
}

vector<Edge> Graph::edges() const {
    vector<Edge> result;
    result.reserve(edgeCount_);
    for (const auto& entry : adjacency_) {
        for (const Edge& edge : entry.second) {
            if (edge.sourceId < edge.destinationId) {
                result.push_back(edge);
            }
        }
    }
    return result;
}

size_t Graph::edgeCount() const noexcept { return edgeCount_; }

void Graph::setDeviceActive(int id, bool isActive) {
    const auto found = devices_.find(id);
    if (found == devices_.end()) {
        throw out_of_range("Device ID does not exist.");
    }
    found->second.isActive = isActive;
}

void Graph::validateDevice(const Device& device) {
    if (device.id < 0) {
        throw invalid_argument("Device ID must be non-negative.");
    }
    if (device.name.empty() || device.ipAddress.empty() ||
        device.type.empty()) {
        throw invalid_argument("Device name, IP address, and type are required.");
    }
    validateScore(device.vulnerabilityScore, "vulnerability_score");
    validateScore(device.exposureScore, "exposure_score");
    validateScore(device.riskScore, "risk_score");
}

void Graph::validateScore(double score, const char* fieldName) {
    if (!isfinite(score) || score < kMinimumComponentScore ||
        score > kMaximumComponentScore) {
        throw invalid_argument(string(fieldName) +
                                    " must be between 0 and 100.");
    }
}

}  // namespace cyberpath
