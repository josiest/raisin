// Use standard library features if they're available
#if __cplusplus > 202002L
#include <ranges>
namespace raisin {

namespace ranges = std::ranges;
namespace views = std::views;
}
// Otherwise use external dependencies
#else
#include <range/v3/all.hpp>

namespace raisin {

namespace ranges = ranges;
namespace views = ranges::views;
}
#endif
