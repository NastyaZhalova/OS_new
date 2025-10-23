#include "Factorial.h"
#include <limits>

std::vector<long long> GenerateFactorials(int n) {
    if (n == 0) {
        throw std::invalid_argument("n must be a positive integer (n >= 1)");
    }

    std::vector<long long> result;
    result.reserve(n);

    long long fact = 1;
    for (int i = 1; i <= n; ++i) {
        if (fact > std::numeric_limits<long long>::max() / i) {
            throw std::overflow_error("Factorial value exceeds maximum representable value");
        }
        fact *= i;
        result.push_back(fact);
    }

    return result;
}
