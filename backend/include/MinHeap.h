#pragma once

#include <cstddef>
#include <vector>

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
    std::size_t size() const noexcept;

private:
    std::vector<HeapEntry> entries_;

    static bool hasHigherPriority(const HeapEntry& lhs, const HeapEntry& rhs);
    void siftUp(std::size_t index);
    void siftDown(std::size_t index);
};

}  // namespace cyberpath
