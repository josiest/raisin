#pragma once
#include "raisin/future.hpp"

// low-level frameworks
#include <SDL2/SDL.h>

// data types
#include <string>
#include <cstdint>

// algorithms
#include <algorithm>
#include <iterator>
#include <concepts>
#include <functional>

// data structures/resource handles
#include <unordered_map>
#include <optional>
#include <tl/expected.hpp>

// serialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>

namespace raisin {

using namespace std::string_literals;
using flag_lookup = std::unordered_map<std::string, std::uint32_t>;

std::string
inline _strlower(std::string const & str)
{
    auto tolower = [](unsigned char ch) { return std::tolower(ch); };
    return str | views::transform(tolower) | ranges::to<std::string>();
}

/**
 * Parse a range of flag names names into their respective SDL flags
 *
 * \param lookup - maps flag names to their respective SDL flags
 * \param flag_names - the names to parse
 * \param invalid_names - where to write invalid names
 *
 * \return the union of the flags as integers. Any undefined flag names won't be
 *         included in the union, but will be written to invalid_names.
 */
template<ranges::input_range name_reader,
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
    auto lower_names = flag_names | views::transform(_strlower)
                                  | ranges::to<std::vector>;

    // write down any invalid names
    auto name_is_invalid = std::not_fn(name_is_valid);
    ranges::copy_if(lower_names, invalid_names, name_is_invalid);

    // transform valid names into flags and join
    auto join = [](std::uint32_t sum, std::uint32_t x) { return sum | x; };
    return ranges::accumulate(lower_names | views::filter(name_is_valid)
                                          | views::transform(as_flag),
                               0u, join);
}

/**
 * Load SDL flag names from a config file into a writable iterator.
 *
 * Parameters:
 *   \param config_path - the name of the path to load
 *   \param variable_path - the toml variable path to load from
 *   \param flag_names - the flag names to write to
 *
 * \return the number of names written on succesfully parsing the config file.
 *         Otherwise, return a parse
 */
template<std::weakly_incrementable name_writer>
tl::expected<std::size_t, std::string>

load_flag_names(std::string const & config_path,
                std::string const & variable_path,
                name_writer flag_names)
{
    // read the config file, return any parsing errors
    toml::parse_result result = toml::parse_file(config_path);
    if (not result) {
        return tl::unexpected(std::string(result.error().description()));
    }
    auto table = std::move(result).table();

    // if subsystem-flags aren't specified then there are no susbystems written
    auto flag_node = table.at_path(variable_path);
    if (not flag_node) {
        return 0u;
    }
    // flag_names must be an array of strings
    if (not flag_node.is_array()) {
        return tl::unexpected(variable_path + " must be an array"s);
    }
    auto flags = *flag_node.as_array();
    if (not flags.is_homogeneous(toml::node_type::string)) {
        return tl::unexpected("all subsytem names in " + variable_path + " "s +
                              "must be strings"s);
    }
    // copy the flags as strings into the subsystem names
    flags.for_each([&flag_names](toml::value<std::string> const & name) {
        *flag_names = name.get();
        ++flag_names;
    });
    return flags.size();
}

}
