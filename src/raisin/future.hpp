namespace raisin {

// Use C++23 standard library features if they're available
#if __cplusplus > 202300L
#include <expected>

template<class T, class E>
using expected = std::expected<T, E>;
template<class T, class E>
using unexpected = std::unexpected<T, E>;

// Otherwise use external dependencies
#else
#include <tl/expected.hpp>
template<class T, class E>
using expected = tl::expected<T, E>;
template<class E>
using unexpected = tl::unexpected<E>;

#endif
}
