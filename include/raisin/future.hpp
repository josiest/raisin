// Use standard library features if they're available
#if __cplusplus > 202002L
#include <ranges>
namespace sdl {

namespace ranges = std::ranges;
namespace views = std::views;
}
// Otherwise use external dependencies
#else
#include <range/v3/range.hpp>
#include <range/v3/view.hpp>

namespace sdl {

namespace ranges = ranges;
namespace views = ranges::views;
}
#endif

