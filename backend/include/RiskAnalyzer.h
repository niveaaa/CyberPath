#pragma once

#include "Device.h"
#include "Graph.h"

#include <vector>

using namespace std;

namespace cyberpath {

enum class RiskCategory { Low, Moderate, High };

struct RiskAssessment {
    Device device;
    RiskCategory category = RiskCategory::Low;
};

// Inputs are already normalized to the documented 0-100 scale and are
// validated at graph boundaries. The formula returns a score in that scale.
double riskScore(const Device& device);
RiskCategory riskLevel(double score);
const char* riskCategoryName(RiskCategory category) noexcept;
RiskAssessment assessRisk(const Device& device);
vector<RiskAssessment> rankByRisk(const Graph& graph);

}  // namespace cyberpath
