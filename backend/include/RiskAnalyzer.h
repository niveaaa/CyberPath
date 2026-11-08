#pragma once

#include "Device.h"
#include "Graph.h"

#include <vector>

namespace cyberpath {

enum class RiskCategory { Low, Moderate, High };

struct RiskAssessment {
    Device device;
    RiskCategory category = RiskCategory::Low;
};

// Inputs are already normalized to the documented 0-100 scale and are
// validated at graph boundaries. The formula returns a score in that scale.
double calculateRiskScore(const Device& device);
RiskCategory classifyRisk(double score);
const char* riskCategoryName(RiskCategory category) noexcept;
RiskAssessment assessRisk(const Device& device);
std::vector<RiskAssessment> rankRisk(const Graph& graph);

}  // namespace cyberpath
