#pragma once

#include "Device.h"
#include "Edge.h"
#include "HashTable.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace std;

namespace cyberpath {

// An undirected graph backed by adjacency lists. Each logical connection is
// stored in both endpoint lists; neighbors are kept in ascending ID order.
class Graph {
public:
    void addDevice(const Device& device);
    bool removeDevice(int id);
    bool containsDevice(int id) const noexcept;
    const Device& device(int id) const;
    optional<int> deviceIdByName(const string& name) const;
    optional<int> deviceIdByIpAddress(const string& ipAddress) const;
    vector<Device> devices() const;
    size_t deviceCount() const noexcept;

    void addEdge(int sourceId, int destinationId, double cost,
                 bool isActive = true);
    bool removeEdge(int sourceId, int destinationId);
    bool containsEdge(int sourceId, int destinationId) const;
    void setEdgeActive(int sourceId, int destinationId, bool isActive);
    const vector<Edge>& neighbors(int id) const;
    vector<Edge> edges() const;
    size_t edgeCount() const noexcept;

    void setDeviceActive(int id, bool isActive);
    void setRiskAttributes(int id, const RiskAttributes& attributes);

private:
    map<int, Device> devices_;
    map<int, vector<Edge>> adjacency_;
    HashTable<string, int> deviceIdsByName_;
    HashTable<string, int> deviceIdsByIpAddress_;
    size_t edgeCount_ = 0;

    static void validateDevice(const Device& device);
    static void validateScore(double score, const char* fieldName);
};

}  // namespace cyberpath
