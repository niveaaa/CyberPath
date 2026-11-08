#pragma once

#include <cstddef>
#include <vector>

using namespace std;

namespace cyberpath {

struct HeapEntry {
    double priority = 0.0;
    int deviceId = 0;
};

// Binary min-heap ordered by priority, then by device ID for stable ties.
class MinHeap {
public:
    void push(const HeapEntry& entry);
    const HeapEntry& top() const;
    HeapEntry pop();
    bool empty() const noexcept;
    size_t size() const noexcept;

private:
    vector<HeapEntry> entries_;

    static bool hasHigherPriority(const HeapEntry& lhs, const HeapEntry& rhs);
    void siftUp(size_t index);
    void siftDown(size_t index);
};

}  // namespace cyberpath
