#pragma once

// frameworks
#include "raisin/future/expected.hpp"
#include "raisin/lookup_table.hpp"

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

namespace limits {
std::size_t constexpr max_flags = 32;
}

inline std::string _strlower(std::string const & str)
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
    std::ranges::borrowed_subrange_t<input> invalid_names;
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
          std::convertible_to<std::ranges::range_value_t<input>, std::string> and
          std::unsigned_integral<std::invoke_result_t<projection, std::string>>)

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

template<typename table_t>
concept flag_lookup = searchable<table_t> and
                      pair_iterator<search_iterator_t<table_t>> and
                      std::convertible_to<std::string, lookup_key_t<table_t>> and
                      std::unsigned_integral<lookup_value_t<table_t>>;

template<std::ranges::forward_range input, flag_lookup table_t>
requires std::permutable<std::ranges::iterator_t<input>> and
         std::convertible_to<std::ranges::range_value_t<input>,
                             lookup_key_t<table_t>>

flag_result<lookup_value_t<table_t>, input>
parse_flags(input && flag_names, table_t const & flagmap)
{
    auto is_flag = [&flagmap](std::string const & name)
    {
        return flagmap.find(name) != flagmap.end();
    };
    auto as_flag = [&flagmap](std::string const & name)
    {
        return flagmap.find(name)->second;
    };
    return parse_flags(std::forward<input>(flag_names), is_flag, as_flag);
}

template<std::size_t max_flags = limits::max_flags,
         flag_lookup lookup_t,
         std::weakly_incrementable name_output>
requires std::indirectly_writable<name_output, std::string>

expected<lookup_value_t<lookup_t>, std::string>
load_flags(toml::table const & table,
           std::string const & variable_path,
           lookup_t const & flagmap,
           name_output into_invalid_names)
{
    namespace ranges = std::ranges;

    std::array<std::string, max_flags> flag_names;
    auto end_names = load_array(table, variable_path, flag_names);
    if (not end_names) {
        return unexpected{ end_names.error() };
    }

    auto loaded_names = ranges::subrange{ ranges::begin(flag_names),
                                          *end_names };
    auto result = parse_flags(loaded_names, flagmap);

    ranges::copy(result.invalid_names, into_invalid_names);
    return result.value;
}

template<std::size_t max_flags = limits::max_flags,
         flag_lookup lookup_t>

expected<lookup_value_t<lookup_t>, std::string>
load_flags(toml::table const & table,
           std::string const & variable_path,
           lookup_t const & flagmap)
{
    namespace ranges = std::ranges;

    std::array<std::string, max_flags> flag_names;
    auto end_names = load_array(table, variable_path, flag_names);
    if (not end_names) {
        return unexpected{ end_names.error() };
    }

    auto loaded_names = ranges::subrange{ ranges::begin(flag_names),
                                          *end_names };
    return parse_flags(loaded_names, flagmap).value;
}

template<std::unsigned_integral flag_t,
         std::weakly_incrementable name_output,
         std::invocable<toml::table, std::string, name_output> load_fn>
requires std::indirectly_writable<name_output, std::string>

auto _load_flags(load_fn && load_flags,
                 std::string const & variable_path,
                 flag_t & output,
                 name_output into_invalid_names)
{
    return [&load_flags, &variable_path, &output, into_invalid_names]
           (toml::table const & table)
        -> expected<toml::table, std::string>
    {
        auto result = std::invoke(std::forward<load_fn>(load_flags),
                                  table, variable_path,
                                  into_invalid_names);
        if (not result) {
            return unexpected(result.error());
        }
        output = *result;
        return table;
    };
}
}
