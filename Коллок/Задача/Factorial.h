#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

namespace math {

    class Factorial {
    public:
        static std::vector<std::uint64_t> generate(std::size_t n);
    };

}
