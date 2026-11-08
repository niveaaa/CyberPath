#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cyberpath {

struct SearchResult {
    std::optional<std::size_t> index;
    std::size_t comparisons = 0;

    bool found() const noexcept { return index.has_value(); }
};

template <typename T, typename Less = std::less<T>>
class SortedData {
public:
    explicit SortedData(std::vector<T> values, Less less = Less{})
        : values_(std::move(values)), less_(std::move(less)) {
        for (std::size_t index = 1; index < values_.size(); ++index) {
            if (less_(values_[index], values_[index - 1])) {
                throw std::invalid_argument(
                    "Binary search requires data ordered by its comparator.");
            }
        }
    }

    const std::vector<T>& values() const noexcept { return values_; }
    const Less& less() const noexcept { return less_; }

private:
    std::vector<T> values_;
    Less less_;
};

// Comparisons count one equality check for each inspected element.
template <typename T, typename Equal = std::equal_to<T>>
SearchResult linearSearch(const std::vector<T>& values, const T& target,
                          Equal equal = Equal{}) {
    SearchResult result;
    for (std::size_t index = 0; index < values.size(); ++index) {
        ++result.comparisons;
        if (equal(values[index], target)) {
            result.index = index;
            return result;
        }
    }
    return result;
}

// SortedData validates ascending order once at construction. Binary search
// comparison metrics count each less-than comparison performed by the search.
template <typename T, typename Less = std::less<T>>
SearchResult binarySearch(const SortedData<T, Less>& data, const T& target) {
    SearchResult result;
    const auto& values = data.values();
    const Less& less = data.less();
    std::size_t low = 0;
    std::size_t high = values.size();

    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
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
        result.index = middle;
        return result;
    }
    return result;
}

}  // namespace cyberpath
