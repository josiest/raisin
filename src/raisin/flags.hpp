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

/**
 * Parse a range of flag names names into a single integer flag
 *
 * \param flag_names        the names to parse
 * \param name_is_valid     determines if a name is a valid flag name
 * \param as_flag           transforms a flag name into an integer
 * \param invalid_names     a place to write invalid names to
 *
 * \return the union of the flags as integers. Any undefined flag names won't be
 *         included in the union, but will be written to invalid_names.
 */
template<std::ranges::input_range input,
         std::predicate<std::string> predicate,
         std::regular_invocable<std::string> projection,
         output_range<std::string> output>

requires (std::convertible_to<std::ranges::range_value_t<input>,
                              std::string> and
          std::integral<std::invoke_result_t<projection, std::string>>)

std::invoke_result_t<projection, std::string>
parse_flags(input const & flag_names,
            predicate name_is_valid,
            projection as_flag,
            output && invalid_names)
{
    namespace ranges = std::ranges;

    // make sure all names are lower-case
    std::array<std::string, MAX_FLAGS> lower_names;
    auto limited_names = std::views::counted(flag_names.begin(), MAX_FLAGS);
    ranges::transform(limited_names, lower_names.begin(), _strlower);

    // write down any invalid names
    auto invalid_parsed = ranges::partition(lower_names, name_is_valid);
    auto const N = std::min(ranges::size(invalid_parsed),
                            ranges::size(invalid_names));

    auto from_invalid_parsed = ranges::begin(invalid_parsed);
    auto into_invalid_names = ranges::begin(invalid_names);
    ranges::copy_n(from_invalid_parsed, N, into_invalid_names);

    // transform valid names into flags and join
    using flag_t = std::invoke_result_t<projection, std::string>;
    return std::transform_reduce(ranges::begin(lower_names),
                                 ranges::begin(invalid_parsed),
                                 0u, std::bit_or<flag_t>{}, as_flag);
}

template<std::unsigned_integral flag_t,
         output_range<std::string> name_output>

expected<flag_t, std::string>
_flags_from_map(std::unordered_map<std::string, flag_t> const & flagmap,
                toml::table const & table,
                std::string const & variable_path,
                name_output && invalid_names)
{
    std::array<std::string, MAX_FLAGS> flag_names;
    auto result = load_array(table, variable_path, flag_names);
    if (not result) {
        return unexpected(result.error());
    }

    auto is_flag = [&flagmap](auto const & name) {
        return flagmap.contains(name);
    };
    auto as_flag = [&flagmap](auto const & name) {
        return flagmap.at(name);
    };

    return parse_flags(flag_names, is_flag, as_flag,
                       std::forward<name_output>(invalid_names));
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
