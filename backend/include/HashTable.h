#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

namespace cyberpath {

// Separate-chaining hash table. Collision count records each new-key insert
// into a bucket that was already occupied; updates and rehashing do not count.
template <typename Key, typename Value, typename Hasher = hash<Key>>
class HashTable {
public:
    explicit HashTable(size_t initialBucketCount = 8,
                       Hasher hasher = Hasher{})
        : buckets_(initialBucketCount), hasher_(move(hasher)) {
        if (initialBucketCount == 0) {
            throw invalid_argument("Hash table needs at least one bucket.");
        }
    }

    // Returns true when a key is newly inserted, false when its value is updated.
    bool insert(const Key& key, const Value& value) {
        auto& bucket = buckets_[bucketIndex(key)];
        for (auto& entry : bucket) {
            if (entry.first == key) {
                entry.second = value;
                return false;
            }
        }

        if (!bucket.empty()) {
            ++collisionCount_;
        }
        bucket.emplace_back(key, value);
        ++size_;
        if (loadFactor() > kMaximumLoadFactor) {
            rehash(buckets_.size() * 2);
        }
        return true;
    }

    Value* find(const Key& key) {
        auto& bucket = buckets_[bucketIndex(key)];
        for (auto& entry : bucket) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    const Value* find(const Key& key) const {
        const auto& bucket = buckets_[bucketIndex(key)];
        for (const auto& entry : bucket) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    bool contains(const Key& key) const { return find(key) != nullptr; }

    bool erase(const Key& key) {
        auto& bucket = buckets_[bucketIndex(key)];
        for (auto entry = bucket.begin(); entry != bucket.end(); ++entry) {
            if (entry->first == key) {
                bucket.erase(entry);
                --size_;
                return true;
            }
        }
        return false;
    }

    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    size_t collisionCount() const noexcept { return collisionCount_; }
    size_t bucketCount() const noexcept { return buckets_.size(); }
    double loadFactor() const noexcept {
        return static_cast<double>(size_) / static_cast<double>(buckets_.size());
    }

private:
    static constexpr double kMaximumLoadFactor = 0.75;
    using Entry = pair<Key, Value>;

    vector<vector<Entry>> buckets_;
    Hasher hasher_;
    size_t size_ = 0;
    size_t collisionCount_ = 0;

    size_t bucketIndex(const Key& key) const {
        return static_cast<size_t>(hasher_(key)) % buckets_.size();
    }

    void rehash(size_t newBucketCount) {
        vector<vector<Entry>> resized(newBucketCount);
        for (auto& bucket : buckets_) {
            for (auto& entry : bucket) {
                const size_t index =
                    static_cast<size_t>(hasher_(entry.first)) % newBucketCount;
                resized[index].push_back(move(entry));
            }
        }
        buckets_ = move(resized);
    }
};

}  // namespace cyberpath
