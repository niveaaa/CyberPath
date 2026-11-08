#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace cyberpath {

struct SortMetrics {
    std::size_t comparisons = 0;
    std::size_t moves = 0;
    std::size_t swaps = 0;
};

template <typename T>
struct SortResult {
    std::vector<T> values;
    SortMetrics metrics;
};

namespace detail {

template <typename T, typename Less>
void countedSwap(std::vector<T>& values, std::size_t lhs, std::size_t rhs,
                 SortMetrics& metrics) {
    if (lhs == rhs) {
        return;
    }
    using std::swap;
    swap(values[lhs], values[rhs]);
    ++metrics.swaps;
    metrics.moves += 3;
}

}  // namespace detail

// Comparison = one comparator call. Swap = one exchange of distinct positions;
// each swap counts three moves. Initial input copying is excluded. Insertion
// moves include key temporaries and shifts; merge moves include buffer writes
// and copy-back assignments; quicksort moves include pivot copies and swaps.
template <typename T, typename Less = std::less<T>>
SortResult<T> bubbleSort(const std::vector<T>& input, Less less = Less{}) {
    SortResult<T> result{input, {}};
    for (std::size_t end = result.values.size(); end > 1; --end) {
        bool changed = false;
        for (std::size_t index = 1; index < end; ++index) {
            ++result.metrics.comparisons;
            if (less(result.values[index], result.values[index - 1])) {
                detail::countedSwap<T, Less>(result.values, index, index - 1,
                                             result.metrics);
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }
    return result;
}

template <typename T, typename Less = std::less<T>>
SortResult<T> insertionSort(const std::vector<T>& input, Less less = Less{}) {
    SortResult<T> result{input, {}};
    for (std::size_t index = 1; index < result.values.size(); ++index) {
        T key = std::move(result.values[index]);
        ++result.metrics.moves;
        std::size_t position = index;
        while (position > 0) {
            ++result.metrics.comparisons;
            if (!less(key, result.values[position - 1])) {
                break;
            }
            result.values[position] = std::move(result.values[position - 1]);
            ++result.metrics.moves;
            --position;
        }
        result.values[position] = std::move(key);
        ++result.metrics.moves;
    }
    return result;
}

template <typename T, typename Less = std::less<T>>
SortResult<T> mergeSort(const std::vector<T>& input, Less less = Less{}) {
    SortResult<T> result{input, {}};
    const std::size_t count = result.values.size();
    if (count < 2) {
        return result;
    }
    std::vector<T> buffer(count);

    for (std::size_t width = 1; width < count;) {
        for (std::size_t start = 0; start < count; start += width * 2) {
            const std::size_t middle = std::min(start + width, count);
            const std::size_t end = std::min(start + width * 2, count);
            std::size_t left = start;
            std::size_t right = middle;
            std::size_t output = start;

            while (left < middle && right < end) {
                ++result.metrics.comparisons;
                if (less(result.values[right], result.values[left])) {
                    buffer[output++] = result.values[right++];
                } else {
                    buffer[output++] = result.values[left++];
                }
                ++result.metrics.moves;
            }
            while (left < middle) {
                buffer[output++] = result.values[left++];
                ++result.metrics.moves;
            }
            while (right < end) {
                buffer[output++] = result.values[right++];
                ++result.metrics.moves;
            }
            for (std::size_t index = start; index < end; ++index) {
                result.values[index] = buffer[index];
                ++result.metrics.moves;
            }
        }
        if (width > count / 2) {
            break;
        }
        width *= 2;
    }
    return result;
}

// Iterative Hoare partitioning with the middle value as pivot. It avoids
// recursion depth growth; worst-case comparison time remains O(n^2).
template <typename T, typename Less = std::less<T>>
SortResult<T> quickSort(const std::vector<T>& input, Less less = Less{}) {
    SortResult<T> result{input, {}};
    if (result.values.size() < 2) {
        return result;
    }

    using Index = std::ptrdiff_t;
    std::vector<std::pair<Index, Index>> ranges{
        {0, static_cast<Index>(result.values.size() - 1)}};
    while (!ranges.empty()) {
        const auto range = ranges.back();
        ranges.pop_back();
        Index left = range.first;
        Index right = range.second;
        T pivot = result.values[static_cast<std::size_t>(
            left + (right - left) / 2)];
        ++result.metrics.moves;

        while (left <= right) {
            while (true) {
                ++result.metrics.comparisons;
                if (!less(result.values[static_cast<std::size_t>(left)], pivot)) {
                    break;
                }
                ++left;
            }
            while (true) {
                ++result.metrics.comparisons;
                if (!less(pivot, result.values[static_cast<std::size_t>(right)])) {
                    break;
                }
                --right;
            }
            if (left <= right) {
                detail::countedSwap<T, Less>(
                    result.values, static_cast<std::size_t>(left),
                    static_cast<std::size_t>(right), result.metrics);
                ++left;
                --right;
            }
        }

        if (range.first < right) {
            ranges.emplace_back(range.first, right);
        }
        if (left < range.second) {
            ranges.emplace_back(left, range.second);
        }
    }
    return result;
}

}  // namespace cyberpath
