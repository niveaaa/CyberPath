#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

using namespace std;

namespace cyberpath {

struct SortMetrics {
    size_t comparisons = 0;
    size_t moves = 0;
    size_t swaps = 0;
};

template <typename T>
struct SortResult {
    vector<T> values;
    SortMetrics metrics;
};

namespace detail {

template <typename T, typename Less>
void countedSwap(vector<T>& values, size_t lhs, size_t rhs,
                 SortMetrics& metrics) {
    if (lhs == rhs) {
        return;
    }

    swap(values[lhs], values[rhs]);
    ++metrics.swaps;
    metrics.moves += 3;
}

}  // namespace detail

// Comparison = one comparator call. Swap = one exchange of distinct positions;
// each swap counts three moves. Initial input copying is excluded. Insertion
// moves include key temporaries and shifts; merge moves include buffer writes
// and copy-back assignments; quicksort moves include pivot copies and swaps.
template <typename T, typename Less = less<T>>
SortResult<T> bubbleSort(const vector<T>& input, Less less = Less{}) {
    SortResult<T> result{input, {}};
    for (size_t end = result.values.size(); end > 1; --end) {
        bool changed = false;
        for (size_t index = 1; index < end; ++index) {
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

template <typename T, typename Less = less<T>>
SortResult<T> insertionSort(const vector<T>& input, Less less = Less{}) {
    SortResult<T> result{input, {}};
    for (size_t index = 1; index < result.values.size(); ++index) {
        T key = move(result.values[index]);
        ++result.metrics.moves;
        size_t position = index;
        while (position > 0) {
            ++result.metrics.comparisons;
            if (!less(key, result.values[position - 1])) {
                break;
            }
            result.values[position] = move(result.values[position - 1]);
            ++result.metrics.moves;
            --position;
        }
        result.values[position] = move(key);
        ++result.metrics.moves;
    }
    return result;
}

template <typename T, typename Less = less<T>>
SortResult<T> mergeSort(const vector<T>& input, Less less = Less{}) {
    SortResult<T> result{input, {}};
    const size_t count = result.values.size();
    if (count < 2) {
        return result;
    }
    vector<T> buffer(count);

    for (size_t width = 1; width < count;) {
        for (size_t start = 0; start < count; start += width * 2) {
            const size_t middle = min(start + width, count);
            const size_t end = min(start + width * 2, count);
            size_t left = start;
            size_t right = middle;
            size_t output = start;

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
            for (size_t index = start; index < end; ++index) {
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
template <typename T, typename Less = less<T>>
SortResult<T> quickSort(const vector<T>& input, Less less = Less{}) {
    SortResult<T> result{input, {}};
    if (result.values.size() < 2) {
        return result;
    }

    using Index = ptrdiff_t;
    vector<pair<Index, Index>> ranges{
        {0, static_cast<Index>(result.values.size() - 1)}};
    while (!ranges.empty()) {
        const auto range = ranges.back();
        ranges.pop_back();
        Index left = range.first;
        Index right = range.second;
        T pivot = result.values[static_cast<size_t>(
            left + (right - left) / 2)];
        ++result.metrics.moves;

        while (left <= right) {
            while (true) {
                ++result.metrics.comparisons;
                if (!less(result.values[static_cast<size_t>(left)], pivot)) {
                    break;
                }
                ++left;
            }
            while (true) {
                ++result.metrics.comparisons;
                if (!less(pivot, result.values[static_cast<size_t>(right)])) {
                    break;
                }
                --right;
            }
            if (left <= right) {
                detail::countedSwap<T, Less>(
                    result.values, static_cast<size_t>(left),
                    static_cast<size_t>(right), result.metrics);
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
