#include "MinHeap.h"

#include <cmath>
#include <stdexcept>
#include <utility>

using namespace std;

namespace cyberpath {

void MinHeap::push(const HeapEntry& entry) {
    if (!isfinite(entry.priority)) {
        throw invalid_argument("Heap priority must be finite.");
    }
    entries_.push_back(entry);
    siftUp(entries_.size() - 1);
}

const HeapEntry& MinHeap::top() const {
    if (empty()) {
        throw underflow_error("Cannot inspect the top of an empty heap.");
    }
    return entries_.front();
}

HeapEntry MinHeap::pop() {
    if (empty()) {
        throw underflow_error("Cannot remove an item from an empty heap.");
    }
    HeapEntry result = entries_.front();
    if (entries_.size() == 1) {
        entries_.pop_back();
        return result;
    }
    entries_.front() = entries_.back();
    entries_.pop_back();
    siftDown(0);
    return result;
}

bool MinHeap::empty() const noexcept { return entries_.empty(); }

size_t MinHeap::size() const noexcept { return entries_.size(); }

bool MinHeap::hasHigherPriority(const HeapEntry& lhs, const HeapEntry& rhs) {
    if (lhs.priority != rhs.priority) {
        return lhs.priority < rhs.priority;
    }
    return lhs.deviceId < rhs.deviceId;
}

void MinHeap::siftUp(size_t index) {
    while (index > 0) {
        const size_t parent = (index - 1) / 2;
        if (!hasHigherPriority(entries_[index], entries_[parent])) {
            break;
        }
        swap(entries_[index], entries_[parent]);
        index = parent;
    }
}

void MinHeap::siftDown(size_t index) {
    while (true) {
        const size_t left = index * 2 + 1;
        const size_t right = left + 1;
        size_t best = index;

        if (left < entries_.size() &&
            hasHigherPriority(entries_[left], entries_[best])) {
            best = left;
        }
        if (right < entries_.size() &&
            hasHigherPriority(entries_[right], entries_[best])) {
            best = right;
        }
        if (best == index) {
            break;
        }
        swap(entries_[index], entries_[best]);
        index = best;
    }
}

}  // namespace cyberpath
