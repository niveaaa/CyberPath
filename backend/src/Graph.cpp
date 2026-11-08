#include "Graph.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace cyberpath {
namespace {

void eraseNeighbor(std::vector<Edge>& neighbors, int destinationId) {
    const auto found = std::lower_bound(
        neighbors.begin(), neighbors.end(), destinationId,
        [](const Edge& edge, int id) { return edge.destinationId < id; });
    if (found != neighbors.end() && found->destinationId == destinationId) {
        neighbors.erase(found);
    }
}

}  // namespace

void Graph::addDevice(const Device& deviceValue) {
    validateDevice(deviceValue);
    if (devices_.find(deviceValue.id) != devices_.end()) {
        throw std::invalid_argument("Device ID already exists.");
    }
    if (deviceIdsByName_.contains(deviceValue.name)) {
        throw std::invalid_argument("Device name already exists.");
    }
    if (deviceIdsByIpAddress_.contains(deviceValue.ipAddress)) {
        throw std::invalid_argument("Device IP address already exists.");
    }

    devices_.emplace(deviceValue.id, deviceValue);
    adjacency_.emplace(deviceValue.id, std::vector<Edge>{});
    deviceIdsByName_.insert(deviceValue.name, deviceValue.id);
    deviceIdsByIpAddress_.insert(deviceValue.ipAddress, deviceValue.id);
}

bool Graph::removeDevice(int id) {
    const auto found = devices_.find(id);
    if (found == devices_.end()) {
        return false;
    }

    const auto incident = adjacency_.find(id);
    for (const Edge& edge : incident->second) {
        eraseNeighbor(adjacency_.at(edge.destinationId), id);
        --edgeCount_;
    }
    deviceIdsByName_.erase(found->second.name);
    deviceIdsByIpAddress_.erase(found->second.ipAddress);
    adjacency_.erase(incident);
    devices_.erase(found);
    return true;
}

bool Graph::containsDevice(int id) const noexcept {
    return devices_.find(id) != devices_.end();
}

const Device& Graph::device(int id) const {
    const auto found = devices_.find(id);
    if (found == devices_.end()) {
        throw std::out_of_range("Device ID does not exist.");
    }
    return found->second;
}

std::optional<int> Graph::deviceIdByName(const std::string& name) const {
    const int* id = deviceIdsByName_.find(name);
    if (id == nullptr) {
        return std::nullopt;
    }
    return *id;
}

std::optional<int> Graph::deviceIdByIpAddress(
    const std::string& ipAddress) const {
    const int* id = deviceIdsByIpAddress_.find(ipAddress);
    if (id == nullptr) {
        return std::nullopt;
    }
    return *id;
}

std::vector<Device> Graph::devices() const {
    std::vector<Device> result;
    result.reserve(devices_.size());
    for (const auto& entry : devices_) {
        result.push_back(entry.second);
    }
    return result;
}

std::size_t Graph::deviceCount() const noexcept { return devices_.size(); }

void Graph::addEdge(int sourceId, int destinationId, double cost,
                    bool isActive) {
    if (!containsDevice(sourceId) || !containsDevice(destinationId)) {
        throw std::invalid_argument("Both edge endpoints must exist.");
    }
    if (sourceId == destinationId) {
        throw std::invalid_argument("Self-connections are not supported.");
    }
    if (!std::isfinite(cost) || cost < 0.0) {
        throw std::invalid_argument("Edge cost must be finite and non-negative.");
    }
    if (containsEdge(sourceId, destinationId)) {
        throw std::invalid_argument("Connection already exists.");
    }

    const auto insertSorted = [cost, isActive](std::vector<Edge>& list,
                                               int from, int to) {
        const auto position = std::lower_bound(
            list.begin(), list.end(), to,
            [](const Edge& edge, int id) { return edge.destinationId < id; });
        list.insert(position, Edge{from, to, cost, isActive});
    };
    insertSorted(adjacency_.at(sourceId), sourceId, destinationId);
    insertSorted(adjacency_.at(destinationId), destinationId, sourceId);
    ++edgeCount_;
}

bool Graph::removeEdge(int sourceId, int destinationId) {
    if (!containsDevice(sourceId) || !containsDevice(destinationId) ||
        !containsEdge(sourceId, destinationId)) {
        return false;
    }
    eraseNeighbor(adjacency_.at(sourceId), destinationId);
    eraseNeighbor(adjacency_.at(destinationId), sourceId);
    --edgeCount_;
    return true;
}

bool Graph::containsEdge(int sourceId, int destinationId) const {
    const auto source = adjacency_.find(sourceId);
    if (source == adjacency_.end()) {
        return false;
    }
    const auto found = findNeighbor(source->second, destinationId);
    return found != source->second.end() &&
           found->destinationId == destinationId;
}

void Graph::setEdgeActive(int sourceId, int destinationId, bool isActive) {
    if (!containsEdge(sourceId, destinationId)) {
        throw std::out_of_range("Connection does not exist.");
    }
    auto& forward = *findNeighbor(adjacency_.at(sourceId), destinationId);
    auto& reverse = *findNeighbor(adjacency_.at(destinationId), sourceId);
    forward.isActive = isActive;
    reverse.isActive = isActive;
}

const std::vector<Edge>& Graph::neighbors(int id) const {
    const auto found = adjacency_.find(id);
    if (found == adjacency_.end()) {
        throw std::out_of_range("Device ID does not exist.");
    }
    return found->second;
}

std::vector<Edge> Graph::edges() const {
    std::vector<Edge> result;
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

std::size_t Graph::edgeCount() const noexcept { return edgeCount_; }

void Graph::setDeviceActive(int id, bool isActive) {
    const auto found = devices_.find(id);
    if (found == devices_.end()) {
        throw std::out_of_range("Device ID does not exist.");
    }
    found->second.isActive = isActive;
}

void Graph::validateDevice(const Device& deviceValue) {
    if (deviceValue.id < 0) {
        throw std::invalid_argument("Device ID must be non-negative.");
    }
    if (deviceValue.name.empty() || deviceValue.ipAddress.empty() ||
        deviceValue.type.empty()) {
        throw std::invalid_argument("Device name, IP address, and type are required.");
    }
    validateScore(deviceValue.vulnerabilityScore, "vulnerability_score");
    validateScore(deviceValue.exposureScore, "exposure_score");
    validateScore(deviceValue.connectivityScore, "connectivity_score");
    validateScore(deviceValue.criticalityScore, "criticality_score");
    validateScore(deviceValue.riskScore, "risk_score");
}

void Graph::validateScore(double score, const char* fieldName) {
    if (!std::isfinite(score) || score < kMinimumComponentScore ||
        score > kMaximumComponentScore) {
        throw std::invalid_argument(std::string(fieldName) +
                                    " must be between 0 and 100.");
    }
}

std::vector<Edge>::iterator Graph::findNeighbor(std::vector<Edge>& list,
                                                int destinationId) {
    return std::lower_bound(
        list.begin(), list.end(), destinationId,
        [](const Edge& edge, int id) { return edge.destinationId < id; });
}

std::vector<Edge>::const_iterator Graph::findNeighbor(
    const std::vector<Edge>& list, int destinationId) {
    return std::lower_bound(
        list.begin(), list.end(), destinationId,
        [](const Edge& edge, int id) { return edge.destinationId < id; });
}

}  // namespace cyberpath
