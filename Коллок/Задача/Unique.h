#pragma once
#include <vector>
#include <unordered_set>

namespace util {

    template <typename T>
    std::vector<T> removeDuplicates(const std::vector<T>& input) {
        std::unordered_set<T> seen;
        seen.reserve(input.size());

        std::vector<T> out;
        out.reserve(input.size());

        for (const auto& v : input) {
            if (seen.insert(v).second)
                out.push_back(v);
        }
        return out;
    }

}
