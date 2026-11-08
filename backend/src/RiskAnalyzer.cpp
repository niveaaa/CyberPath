#include "RiskAnalyzer.h"

#include "Sorting.h"

#include <cmath>
#include <stdexcept>
#include <string>

using namespace std;

namespace cyberpath {
namespace {

void validateScore(double score, const char* fieldName) {
    if (!isfinite(score) || score < kMinimumComponentScore ||
        score > kMaximumComponentScore) {
        throw invalid_argument(string(fieldName) +
                                    " must be between 0 and 100.");
    }
}

}  // namespace

double riskScore(const Device& device) {
    validateScore(device.vulnerabilityScore, "vulnerability_score");
    validateScore(device.exposureScore, "exposure_score");
    return (device.vulnerabilityScore + device.exposureScore) / 2.0;
}

RiskCategory riskLevel(double score) {
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
    assessment.device.riskScore = riskScore(device);
    assessment.category = riskLevel(assessment.device.riskScore);
    return assessment;
}

vector<RiskAssessment> rankByRisk(const Graph& graph) {
    vector<RiskAssessment> assessments;
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
