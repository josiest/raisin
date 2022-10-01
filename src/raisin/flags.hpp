#pragma once

// frameworks
#include "raisin/future.hpp"

// data types
#include <string>
#include <cstdint>

// algorithms
#include <algorithm>    // copy_if, transform
#include <numeric>      // accumulate

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

std::string
inline _strlower(std::string const & str)
{
    auto tolower = [](unsigned char ch) { return std::tolower(ch); };
    std::string lower{};
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
template<std::ranges::input_range name_reader,
         std::predicate<std::string> name_validator,
         std::regular_invocable<std::string> name_transformer,
         std::weakly_incrementable name_writer>

std::uint32_t
parse_flags(name_reader const & flag_names,
            name_validator name_is_valid,
            name_transformer as_flag,
            name_writer invalid_names)
{
    // make sure all names are lower-case
    std::vector<std::string> lower_names;
    std::ranges::transform(flag_names, std::back_inserter(lower_names),
                           _strlower);

    // write down any invalid names
    auto name_is_invalid = std::not_fn(name_is_valid);
    std::ranges::copy_if(lower_names, invalid_names, name_is_invalid);

    // transform valid names into flags and join
    auto join = [](std::uint32_t flag_union, std::uint32_t flag) {
        return flag_union | flag;
    };
    auto to_valid_flags = lower_names | std::views::filter(name_is_valid)
                                      | std::views::transform(as_flag);
    return std::accumulate(to_valid_flags.begin(),
                           to_valid_flags.end(), 0u, join);
}

template<std::weakly_incrementable name_writer>
auto
_flags_from_map(std::unordered_map<std::string, std::uint32_t> const & flagmap,
                std::string const & variable_path,
                std::uint32_t & flag_output,
                name_writer invalid_names)
{
    return [&flagmap, &flag_output, &variable_path, invalid_names]
           (toml::table const & table)
        -> expected<toml::table, std::string>
    {
        std::vector<std::string> flags;
        auto result = load_array<std::string>(table, variable_path,
                                              std::back_inserter(flags));
        if (not result) { return result; }

        auto flag_exists = [&flagmap](auto const & name) {
            return flagmap.contains(name);
        };
        auto as_flag = [&flagmap](auto const & name) {
            return flagmap.at(name);
        };
        flag_output = parse_flags(flags, flag_exists, as_flag, invalid_names);
        return result;
    };
}
}
