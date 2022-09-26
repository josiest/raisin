#pragma once
#include "raisin/future.hpp"

// low-level frameworks
#include <SDL2/SDL.h>

// data types
#include <string>
#include <cstdint>

// algorithms
#include <algorithm>
#include <numeric>
#include <functional>

// ranges and concepts
#include <iterator>
#include <concepts>
#include <ranges>

// data structures/resource handles
#include <unordered_map>
#include <optional>

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
    std::string lower;
    std::ranges::transform(str, std::back_inserter(lower), tolower);
    return lower;
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
    auto join = [](std::uint32_t sum, std::uint32_t x) { return sum | x; };
    auto to_valid_flags = lower_names | std::views::filter(name_is_valid)
                                      | std::views::transform(as_flag);
    return std::accumulate(to_valid_flags.begin(),
                           to_valid_flags.end(), 0u, join);
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
expected<std::size_t, std::string>

load_flag_names(fs::path const & config_path,
                std::string const & variable_path,
                name_writer flag_names)
{
    if (not fs::exists(config_path)) {
        return unexpected("no file named "s + config_path.string());
    }

    // read the config file, return any parsing errors
    toml::parse_result result = toml::parse_file(config_path.string());
    if (not result) {
        return unexpected(std::string(result.error().description()));
    }
    auto table = std::move(result).table();

    // if subsystem-flags aren't specified then there are no susbystems written
    auto flag_node = table.at_path(variable_path);
    if (not flag_node) {
        return 0u;
    }
    // flag_names must be an array of strings
    if (not flag_node.is_array()) {
        return unexpected(variable_path + " must be an array"s);
    }
    auto flags = *flag_node.as_array();
    if (not flags.is_homogeneous(toml::node_type::string)) {
        return unexpected("all subsytem names in " + variable_path + " "s +
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
