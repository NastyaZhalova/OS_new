#include "Factorial.h"
#include <stdexcept>
#include <limits>

namespace math {

    std::vector<std::uint64_t> Factorial::generate(std::size_t n) {
        if (n == 0)
            throw std::invalid_argument("n must be >= 1");

        std::vector<std::uint64_t> out;
        out.reserve(n);

        std::uint64_t cur = 1;
        for (std::size_t i = 1; i <= n; ++i) {
            if (i > 1 && cur > std::numeric_limits<std::uint64_t>::max() / i)
                throw std::overflow_error("factorial overflow");

            cur *= i;
            out.push_back(cur);
        }
        return out;
    }

}
