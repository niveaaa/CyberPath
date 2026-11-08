#include "RiskAnalyzer.h"

#include "Sorting.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace cyberpath {
namespace {

void validateScore(double score, const char* fieldName) {
    if (!std::isfinite(score) || score < kMinimumComponentScore ||
        score > kMaximumComponentScore) {
        throw std::invalid_argument(std::string(fieldName) +
                                    " must be between 0 and 100.");
    }
}

}  // namespace

double calculateRiskScore(const Device& device) {
    validateScore(device.vulnerabilityScore, "vulnerability_score");
    validateScore(device.exposureScore, "exposure_score");
    validateScore(device.connectivityScore, "connectivity_score");
    validateScore(device.criticalityScore, "criticality_score");

    return 0.40 * device.vulnerabilityScore +
           0.25 * device.exposureScore +
           0.20 * device.connectivityScore +
           0.15 * device.criticalityScore;
}

RiskCategory classifyRisk(double score) {
    validateScore(score, "risk_score");
    if (score < 34.0) {
        return RiskCategory::Low;
    }
    if (score < 67.0) {
        return RiskCategory::Moderate;
    }
    return RiskCategory::High;
}

const char* riskCategoryName(RiskCategory category) noexcept {
    switch (category) {
        case RiskCategory::Low:
            return "Low";
        case RiskCategory::Moderate:
            return "Moderate";
        case RiskCategory::High:
            return "High";
    }
    return "Unknown";
}

RiskAssessment assessRisk(const Device& device) {
    RiskAssessment assessment;
    assessment.device = device;
    assessment.device.riskScore = calculateRiskScore(device);
    assessment.category = classifyRisk(assessment.device.riskScore);
    return assessment;
}

std::vector<RiskAssessment> rankRisk(const Graph& graph) {
    std::vector<RiskAssessment> assessments;
    assessments.reserve(graph.deviceCount());
    for (const Device& device : graph.devices()) {
        assessments.push_back(assessRisk(device));
    }
    const auto byDescendingRisk = [](const RiskAssessment& lhs,
                                     const RiskAssessment& rhs) {
        if (lhs.device.riskScore != rhs.device.riskScore) {
            return lhs.device.riskScore > rhs.device.riskScore;
        }
        return lhs.device.id < rhs.device.id;
    };
    return quickSort(assessments, byDescendingRisk).values;
}

}  // namespace cyberpath
