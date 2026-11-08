#include "HashTable.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

namespace {

int failures = 0;

struct ConstantHash {
    size_t operator()(const string&) const noexcept { return 0; }
};

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

void testCollisionLookupUpdateAndDelete() {
    cyberpath::HashTable<string, int, ConstantHash> table(4);
    check(table.empty() && table.size() == 0 && table.loadFactor() == 0.0,
          "new hash table has no entries");
    check(table.insert("alpha", 1), "insert reports a new key");
    check(table.insert("beta", 2), "second key is inserted into its chain");
    check(table.insert("gamma", 3), "third key is inserted into its chain");
    check(table.collisionCount() == 2,
          "each insertion into an occupied bucket counts one collision");
    check(table.contains("alpha") && table.contains("beta") &&
              table.contains("gamma"),
          "contains finds all chained keys");
    check(table.find("beta") != nullptr && *table.find("beta") == 2,
          "find returns the associated value");
    check(table.find("absent") == nullptr && !table.contains("absent"),
          "missing key lookup returns no value");

    check(!table.insert("beta", 20), "insert reports a value update");
    check(*table.find("beta") == 20 && table.size() == 3 &&
              table.collisionCount() == 2,
          "updating a key changes its value but not size or collision count");
    check(table.erase("beta") && !table.contains("beta"),
          "erase removes a key from the middle of a chain");
    check(table.erase("alpha") && table.erase("gamma"),
          "erase removes head and tail chain entries");
    check(!table.erase("missing") && table.size() == 0,
          "erasing a missing key leaves the table unchanged");
}

void testResizeAndLoadFactor() {
    cyberpath::HashTable<int, string> table(2);
    table.insert(1, "one");
    check(table.loadFactor() == 0.5, "load factor uses size divided by buckets");
    table.insert(2, "two");
    table.insert(3, "three");
    check(table.bucketCount() == 4 && table.size() == 3,
          "table grows when load factor exceeds 0.75");
    check(table.loadFactor() == 0.75,
          "load factor is updated after rehash");
    check(*table.find(1) == "one" && *table.find(2) == "two" &&
              *table.find(3) == "three",
          "rehashing preserves all values");
    expectInvalidArgument([&] { cyberpath::HashTable<int, int> invalid(0); },
                          "zero initial bucket count is rejected");
}

}  // namespace

int main() {
    testCollisionLookupUpdateAndDelete();
    testResizeAndLoadFactor();
    if (failures != 0) {
        cerr << failures << " hash table check(s) failed.\n";
        return 1;
    }
    cout << "All hash table checks passed.\n";
    return 0;
}
