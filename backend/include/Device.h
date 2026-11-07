#pragma once

#include <string>

namespace cyberpath {

inline constexpr double kMinimumComponentScore = 0.0;
inline constexpr double kMaximumComponentScore = 100.0;

struct Device {
    int id = 0;
    std::string name;
    std::string ipAddress;
    std::string type;
    double vulnerabilityScore = 0.0;
    double exposureScore = 0.0;
    double connectivityScore = 0.0;
    double criticalityScore = 0.0;
    double riskScore = 0.0;
    bool isCritical = false;
    bool isActive = true;
};

}  // namespace cyberpath
