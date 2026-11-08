#include "MinHeap.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace cyberpath {

void MinHeap::push(const HeapEntry& entry) {
    if (!std::isfinite(entry.priority)) {
        throw std::invalid_argument("Heap priority must be finite.");
    }
    entries_.push_back(entry);
    siftUp(entries_.size() - 1);
}

const HeapEntry& MinHeap::top() const {
    if (empty()) {
        throw std::underflow_error("Cannot inspect the top of an empty heap.");
    }
    return entries_.front();
}

HeapEntry MinHeap::pop() {
    if (empty()) {
        throw std::underflow_error("Cannot remove an item from an empty heap.");
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

std::size_t MinHeap::size() const noexcept { return entries_.size(); }

bool MinHeap::hasHigherPriority(const HeapEntry& lhs, const HeapEntry& rhs) {
    if (lhs.priority != rhs.priority) {
        return lhs.priority < rhs.priority;
    }
    return lhs.deviceId < rhs.deviceId;
}

void MinHeap::siftUp(std::size_t index) {
    while (index > 0) {
        const std::size_t parent = (index - 1) / 2;
        if (!hasHigherPriority(entries_[index], entries_[parent])) {
            break;
        }
        std::swap(entries_[index], entries_[parent]);
        index = parent;
    }
}

void MinHeap::siftDown(std::size_t index) {
    while (true) {
        const std::size_t left = index * 2 + 1;
        const std::size_t right = left + 1;
        std::size_t best = index;

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
        std::swap(entries_[index], entries_[best]);
        index = best;
    }
}

}  // namespace cyberpath
