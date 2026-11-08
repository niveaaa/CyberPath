#pragma once

#include <string>

using namespace std;

namespace cyberpath {

inline constexpr double kMinimumComponentScore = 0.0;
inline constexpr double kMaximumComponentScore = 100.0;

struct Device {
    int id = 0;
    string name;
    string ipAddress;
    string type;
    double vulnerabilityScore = 0.0;
    double exposureScore = 0.0;
    double riskScore = 0.0;
    bool isActive = true;
};

}  // namespace cyberpath
