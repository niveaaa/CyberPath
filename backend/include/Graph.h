#pragma once

#include "Device.h"
#include "Edge.h"
#include "HashTable.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cyberpath {

// An undirected graph backed by adjacency lists. Each logical connection is
// stored in both endpoint lists; neighbors are kept in ascending ID order.
class Graph {
public:
    void addDevice(const Device& device);
    bool removeDevice(int id);
    bool containsDevice(int id) const noexcept;
    const Device& device(int id) const;
    std::optional<int> deviceIdByName(const std::string& name) const;
    std::optional<int> deviceIdByIpAddress(const std::string& ipAddress) const;
    std::vector<Device> devices() const;
    std::size_t deviceCount() const noexcept;

    void addEdge(int sourceId, int destinationId, double cost,
                 bool isActive = true);
    bool removeEdge(int sourceId, int destinationId);
    bool containsEdge(int sourceId, int destinationId) const;
    void setEdgeActive(int sourceId, int destinationId, bool isActive);
    const std::vector<Edge>& neighbors(int id) const;
    std::vector<Edge> edges() const;
    std::size_t edgeCount() const noexcept;

    void setDeviceActive(int id, bool isActive);
    void setRiskAttributes(int id, const RiskAttributes& attributes);

private:
    std::map<int, Device> devices_;
    std::map<int, std::vector<Edge>> adjacency_;
    HashTable<std::string, int> deviceIdsByName_;
    HashTable<std::string, int> deviceIdsByIpAddress_;
    std::size_t edgeCount_ = 0;

    static void validateDevice(const Device& device);
    static void validateScore(double score, const char* fieldName);
    static std::vector<Edge>::iterator findNeighbor(std::vector<Edge>& list,
                                                   int destinationId);
    static std::vector<Edge>::const_iterator findNeighbor(
        const std::vector<Edge>& list, int destinationId);
};

}  // namespace cyberpath
