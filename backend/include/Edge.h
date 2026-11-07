#pragma once

namespace cyberpath {

struct Edge {
    int sourceId = 0;
    int destinationId = 0;
    double cost = 0.0;
    bool isActive = true;
};

}  // namespace cyberpath
