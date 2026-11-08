#include "Device.h"
#include "Searching.h"
#include "Sorting.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace {

int failures = 0;

void check(bool condition, const string& message) {
    if (!condition) {
        ++failures;
        cerr << "FAIL: " << message << '\n';
    }
}

void expectInvalidArgument(const function<void()>& action,
                           const string& message) {
    try {
        action();
        check(false, message + " (did not throw)");
    } catch (const invalid_argument&) {
    } catch (...) {
        check(false, message + " (threw wrong exception)");
    }
}

template <typename T>
void checkSortedOutput(const cyberpath::SortResult<T>& result,
                       const vector<T>& expected,
                       const string& algorithm) {
    check(result.values == expected, algorithm + " returns expected ordered data");
}

void testLinearSearch() {
    const vector<int> values{8, 3, 5, 3, 10};
    const auto found = cyberpath::linearSearch(values, 5);
    check(found.found && found.index == 2 && found.comparisons == 3,
          "linear search reports first match and comparisons");
    const auto missing = cyberpath::linearSearch(values, 7);
    check(!missing.found && missing.comparisons == 5,
          "linear search reports an absent item");
    const vector<int> empty;
    const auto emptyResult = cyberpath::linearSearch(empty, 1);
    check(!emptyResult.found && emptyResult.comparisons == 0,
          "linear search handles empty input");
}

void testBinarySearch() {
    const cyberpath::SortedData<int> sorted({1, 2, 3, 4, 5, 6, 7, 8});
    const auto found = cyberpath::binarySearch(sorted, 6);
    check(found.found && found.index == 5 && found.comparisons == 5,
          "binary search reports match index and less-than comparisons");
    const auto missing = cyberpath::binarySearch(sorted, 0);
    check(!missing.found && missing.comparisons <= 8,
          "binary search reports absent item in logarithmic probes");
    expectInvalidArgument(
        [] { cyberpath::SortedData<int> invalid({1, 3, 2}); },
        "binary search data wrapper rejects unsorted input");
    const cyberpath::SortedData<int> empty({});
    check(!cyberpath::binarySearch(empty, 1).found,
          "binary search handles empty sorted input");
}

void testSortAlgorithmsOnCommonInputs() {
    const vector<int> input{5, -1, 3, 3, 0, 9, -4};
    const vector<int> expected{-4, -1, 0, 3, 3, 5, 9};
    const auto bubble = cyberpath::bubbleSort(input);
    const auto insertion = cyberpath::insertionSort(input);
    const auto merge = cyberpath::mergeSort(input);
    const auto quick = cyberpath::quickSort(input);
    checkSortedOutput(bubble, expected, "bubble sort");
    checkSortedOutput(insertion, expected, "insertion sort");
    checkSortedOutput(merge, expected, "merge sort");
    checkSortedOutput(quick, expected, "quicksort");
    check(bubble.metrics.comparisons > 0 && bubble.metrics.swaps > 0 &&
              bubble.metrics.moves == bubble.metrics.swaps * 3,
          "bubble sort reports comparisons, swaps, and three moves per swap");
    check(insertion.metrics.comparisons > 0 && insertion.metrics.moves > 0 &&
              insertion.metrics.swaps == 0,
          "insertion sort reports comparisons and moves");
    check(merge.metrics.comparisons > 0 && merge.metrics.moves > 0 &&
              merge.metrics.swaps == 0,
          "merge sort reports comparisons and buffer/copy moves");
    check(quick.metrics.comparisons > 0 && quick.metrics.moves > 0,
          "quicksort reports comparisons and pivot/swap moves");

    const vector<int> empty;
    check(cyberpath::bubbleSort(empty).values.empty(),
          "bubble sort handles empty input");
    check(cyberpath::insertionSort(vector<int>{1}).values ==
              vector<int>{1},
          "insertion sort handles single-element input");
    const vector<int> reversed{5, 4, 3, 2, 1};
    const vector<int> ordered{1, 2, 3, 4, 5};
    check(cyberpath::mergeSort(reversed).values == ordered,
          "merge sort handles reverse-sorted input");
    check(cyberpath::quickSort(vector<int>(100, 7)).values ==
              vector<int>(100, 7),
          "quicksort handles duplicate-heavy input");
}

void testCustomComparatorsAndStability() {
    const vector<pair<int, char>> records{{2, 'a'}, {1, 'b'}, {2, 'c'}};
    const auto byFirst = [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    };
    const auto sorted = cyberpath::mergeSort(records, byFirst);
    check(sorted.values[0].second == 'b' && sorted.values[1].second == 'a' &&
              sorted.values[2].second == 'c',
          "merge sort is stable with a custom comparator");

    cyberpath::Device first;
    first.id = 1;
    first.riskScore = 70;
    cyberpath::Device second;
    second.id = 2;
    second.riskScore = 20;
    const vector<cyberpath::Device> devices{first, second};
    const auto byRisk = [](const auto& lhs, const auto& rhs) {
        return lhs.riskScore < rhs.riskScore;
    };
    const auto sortedDevices = cyberpath::quickSort(devices, byRisk);
    check(sortedDevices.values[0].id == 2 && sortedDevices.values[1].id == 1,
          "generic sorting supports device risk-score comparator");
}

void testSmallPermutationCoverage() {
    for (int count = 0; count <= 6; ++count) {
        vector<int> values(static_cast<size_t>(count));
        iota(values.begin(), values.end(), 0);
        do {
            const auto expected = values;
            vector<int> ordered = expected;
            sort(ordered.begin(), ordered.end());
            check(cyberpath::bubbleSort(values).values == ordered,
                  "bubble sort orders a permutation");
            check(cyberpath::insertionSort(values).values == ordered,
                  "insertion sort orders a permutation");
            check(cyberpath::mergeSort(values).values == ordered,
                  "merge sort orders a permutation");
            check(cyberpath::quickSort(values).values == ordered,
                  "quicksort orders a permutation");
        } while (next_permutation(values.begin(), values.end()));
    }
}

}  // namespace

int main() {
    testLinearSearch();
    testBinarySearch();
    testSortAlgorithmsOnCommonInputs();
    testCustomComparatorsAndStability();
    testSmallPermutationCoverage();
    if (failures != 0) {
        cerr << failures << " search/sort check(s) failed.\n";
        return 1;
    }
    cout << "All search and sort checks passed.\n";
    return 0;
}
