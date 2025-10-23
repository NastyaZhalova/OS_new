#include "deduplicate.h"
#include <unordered_set>
#include <stdexcept>

std::vector<int> RemoveDuplicatesPreserveOrder(const std::vector<int>& input) {
    constexpr size_t MAX_INPUT_SIZE = 1000000;
    if (input.empty()) {
        return {};
    }
    if (input.size() > MAX_INPUT_SIZE) {
        throw std::invalid_argument("Input size exceeds maximum allowed size");
    }

    std::unordered_set<int> seen;
    std::vector<int> result;
    result.reserve(input.size());

    try {
        for (int value : input) {
            if (seen.insert(value).second) {
                result.push_back(value);
            }

            if (result.size() > MAX_INPUT_SIZE) {
                throw std::runtime_error("Result size exceeds safe limit");
            }
        }
    }
    catch (const std::bad_alloc& e) {
        throw std::runtime_error("Memory allocation failed during deduplication");
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Unexpected error during deduplication: " + std::string(e.what()));
    }

    return result;
}