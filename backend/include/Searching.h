#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

namespace cyberpath {

struct SearchResult {
    bool found = false;
    size_t index = 0;
    size_t comparisons = 0;
};

template <typename T, typename Less = less<T>>
class SortedData {
public:
    explicit SortedData(vector<T> values, Less less = Less{})
        : values_(move(values)), less_(move(less)) {
        for (size_t index = 1; index < values_.size(); ++index) {
            if (less_(values_[index], values_[index - 1])) {
                throw invalid_argument(
                    "Binary search requires data ordered by its comparator.");
            }
        }
    }

    const vector<T>& values() const noexcept { return values_; }
    const Less& less() const noexcept { return less_; }

private:
    vector<T> values_;
    Less less_;
};

// Comparisons count one equality check for each inspected element.
template <typename T, typename Equal = equal_to<T>>
SearchResult linearSearch(const vector<T>& values, const T& target,
                          Equal equal = Equal{}) {
    SearchResult result;
    for (size_t index = 0; index < values.size(); ++index) {
        ++result.comparisons;
        if (equal(values[index], target)) {
            result.found = true;
            result.index = index;
            return result;
        }
    }
    return result;
}

// SortedData validates ascending order once at construction. Binary search
// comparison metrics count each less-than comparison performed by the search.
template <typename T, typename Less = less<T>>
SearchResult binarySearch(const SortedData<T, Less>& data, const T& target) {
    SearchResult result;
    const auto& values = data.values();
    const Less& less = data.less();
    size_t low = 0;
    size_t high = values.size();

    while (low < high) {
        const size_t middle = low + (high - low) / 2;
        ++result.comparisons;
        if (less(values[middle], target)) {
            low = middle + 1;
            continue;
        }
        ++result.comparisons;
        if (less(target, values[middle])) {
            high = middle;
            continue;
        }
        result.found = true;
        result.index = middle;
        return result;
    }
    return result;
}

}  // namespace cyberpath
