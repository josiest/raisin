#pragma once

// frameworks
#include "raisin/future/expected.hpp"

// data types
#include <string>
#include <cstdint>

// algorithms
#include <algorithm>    // copy_n, transform, partition
#include <numeric>      // transform_reduce

// type constraints
#include <type_traits>
#include <concepts>

#include <ranges>       // input_range
#include <functional>   // predicate, regular_invocable
#include <iterator>     // weakly_incrementable, back_inserter

// data structures
#include <unordered_map>

// serialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
#include <filesystem>

namespace raisin {

using namespace std::string_literals;
namespace fs = std::filesystem;

std::size_t constexpr MAX_FLAGS = 32;

std::string _strlower(std::string const & str)
{
    auto tolower = [](unsigned char ch) { return std::tolower(ch); };
    std::string lower;
    lower.reserve(str.size());
    std::ranges::transform(str, std::back_inserter(lower), tolower);
    return lower;
}

template<std::unsigned_integral flag_t, std::ranges::range input>
struct flag_result {
    flag_t value;
    std::ranges::subrange<std::ranges::iterator_t<input>> invalid_names;
};

/**
 * Parse a range of flag names names into a single integer flag
 *
 * \param flag_names        the names to parse
 * \param name_is_valid     determines if a name is a valid flag name
 * \param as_flag           transforms a flag name into an integer
 * \param invalid_names     a place to write invalid names to
 *
 * \note names in flag names will be transformed to lowercase and rearranged
 *       so that all invalid flag names are at the end of the array.
 *
 * \return the union of the flags as integers. Any undefined flag names won't be
 *         included in the union, but will be written to invalid_names.
 */
template<std::ranges::forward_range input,
         std::predicate<std::string> predicate,
         std::regular_invocable<std::string> projection>

requires (std::permutable<std::ranges::iterator_t<input>> and
          std::convertible_to<std::ranges::range_value_t<input>,
                              std::string> and
          std::integral<std::invoke_result_t<projection, std::string>>)

flag_result<std::invoke_result_t<projection, std::string>, input>
parse_flags(input && flag_names, predicate is_flag, projection as_flag)
{
    namespace ranges = std::ranges;

    // partition invalid names to the end
    ranges::transform(flag_names, flag_names.begin(), _strlower);
    auto invalid_names = ranges::partition(flag_names, is_flag);

    // transform valid names into flags and join
    using flag_t = std::invoke_result_t<projection, std::string>;
    flag_t const value = std::transform_reduce(
            ranges::begin(flag_names), ranges::begin(invalid_names),
            0u, std::bit_or<flag_t>{}, as_flag);

    return { value, invalid_names };
}

template<std::unsigned_integral flag_t,
         output_range<std::string> name_output>

expected<flag_t, std::string>
_flags_from_map(std::unordered_map<std::string, flag_t> const & flagmap,
                toml::table const & table,
                std::string const & variable_path,
                name_output && invalid_names)
{
    namespace ranges = std::ranges;

    // load the array of flag names
    std::array<std::string, MAX_FLAGS> flag_names;
    auto iter_result = load_array(table, variable_path, flag_names);
    if (not iter_result) {
        return unexpected(iter_result.error());
    }

    auto is_flag = [&flagmap](auto const & name) {
        return flagmap.contains(name);
    };
    auto as_flag = [&flagmap](auto const & name) {
        return flagmap.at(name);
    };

    // parse the loaded names into flags
    auto loaded_names = ranges::subrange(ranges::begin(flag_names),
                                         *iter_result);
    auto flag_result = parse_flags(loaded_names, is_flag, as_flag);

    // write down any invalid names
    auto N = std::min(ranges::size(invalid_names),
                      ranges::size(flag_result.invalid_names));
    ranges::copy_n(ranges::begin(flag_result.invalid_names), N,
                   ranges::begin(invalid_names));
    return flag_result.value;
}

template<std::unsigned_integral flag_t,
         output_range<std::string> name_output,
         std::invocable<toml::table, std::string, name_output> load_fn>

auto _load_flags(load_fn && load_flags,
                 std::string const & variable_path,
                 flag_t & output,
                 name_output && invalid_names)
{
    return [&load_flags, &variable_path, &output, &invalid_names]
           (toml::table const & table)
        -> expected<toml::table, std::string>
    {
        auto result = std::invoke(std::forward<load_fn>(load_flags),
                                  table, variable_path,
                                  std::forward<name_output>(invalid_names));
        if (not result) {
            return unexpected(result.error());
        }
        output = *result;
        return table;
    };
}
}
