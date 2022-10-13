#pragma once
#include "raisin/future.hpp"

// data types
#include <string>

// deserialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
#include <filesystem>
#include <span>

// type constraints
#include <concepts>
#include <type_traits>
#include <typeinfo>

// algorithms
#include <ranges>
#include <iterator>
#include <functional>

// i/o
#include <sstream>

using namespace std::string_literals;

inline namespace raisin {

template<typename value_t>
concept native = (std::is_arithmetic_v<value_t> or
                  std::convertible_to<std::string, value_t>);

// node type for non-readables is none
template<typename value_t>
struct node_type :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::none> {};

// node type for bools
template<>
struct node_type<bool> :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::boolean> {};

// node type for integers
template<std::integral value_t>
    requires (not std::same_as<value_t, bool>) // to avoid ambiguity
struct node_type<value_t> :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::integer> {};

// node type for floating points
template<std::floating_point value_t>
struct node_type<value_t> :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::floating_point> {};

// node type for strings
template<typename value_t>
    requires (std::convertible_to<std::string, value_t> and
              (not std::is_arithmetic_v<value_t>))
struct node_type<value_t> :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::string> {};

template<native value_t>
toml::node_type constexpr node_type_v = node_type<value_t>::value;

/**
 * \brief Parse a toml file into an expected table result
 *
 * \param config_path the path to the config file.
 *
 * \return The parsed toml table if parsing was successful, otherwise the
 *         error message for why parsing failed.
 *
 * \note This is a thin wrapper on toml::parse_file that merely adds some more
 *       descriptive fail conditions, and returns an expected result rather
 *       than toml::parse_result.
 */
expected<toml::table, std::string>
parse_file(std::string const & config_path)
{
    if (not std::filesystem::exists(config_path)) {
        std::string const description =
            "Expecting config at "s + config_path + ", "s
            "but the file doesn't exist"s;
        return unexpected{ description };
    }

    toml::parse_result const table_result = toml::parse_file(config_path);
    if (not table_result) {
        std::string const description{ table_result.error().description() };
        return unexpected{ description };
    }
    return table_result.table();
}

expected<toml::table, std::string>
validate_variable(toml::table const & table,
                  std::string const & variable_path)
{
    if (not table.at_path(variable_path)) {
        std::string const description =
            "Expected the variable "s + variable_path + " to exist, "s +
            "but it doesn't"s;
        return unexpected{ description };
    }
    return table;
}

/**
 * \brief Get a subtable of a toml::table
 *
 * \param table             the table to load a subtable from
 * \param variable_path     the toml path to the subtable
 *
 * \return The subtable, or a descriptive message if failed
 */
expected<toml::table, std::string>
subtable(toml::table const & table, std::string const & variable_path)
{
    // make sure the table has the subtable attribute name
    auto result = validate_variable(table, variable_path);
    if (not result) { return unexpected(result.error()); }

    // make sure the subtable is indeed a table
    toml::table const * subtable = table.at_path(variable_path).as_table();
    if (not subtable) {
        std::string const description =
            "Expecting "s + variable_path + " to be a table, "s +
            "but it wasn't"s;
        return unexpected{ description };
    }
    return *subtable;
}

template<typename value_t>
expected<value_t, std::string>
load_value(toml::table const & table, std::string const & variable_path)
{
    std::string const & description =
        "Loading for type "s + typeid(value_t).name() + " is undefined"s;
    return unexpected{ description };
}

/**
 * \brief Load a native value
 *
 * \param table             the table to load data from
 * \param variable_path     the toml path to the variable to load
 *
 * \return the loaded value, or a descriptive error message on failure
 */
template<native value_t>
expected<value_t, std::string>
load_value(toml::table const & table, std::string const & variable_path)
{
    auto table_result = validate_variable(table, variable_path);
    if (not table_result) {
        return unexpected(table_result.error());
    }

    auto value_result = table.at_path(variable_path).value<value_t>();
    if (not value_result) {
        std::string const description =
            "Expecting "s + variable_path + " to have type "s +
            typeid(value_t).name() + ", but it doesn't"s;
        return unexpected{ description };
    }
    return *value_result;
}

template<typename value_t>
concept value = requires(toml::table const & table,
                         std::string const & variable_path) {
    { load_value<value_t>(table, variable_path) }
      -> std::same_as<expected<value_t, std::string>>;
};

/**
 * \brief Load a native
 *
 * \param variable_path     the toml path to the variable to load
 * \param output            where to write the loaded value to
 *
 * \return a function taking a toml::table and returning an expected table
 *         result, such that the target value is written to val when loading
 *         is successful.
 */
template<value value_t>
auto load(std::string const & variable_path, value_t & output)
{
    return [&variable_path, &output](toml::table const & table)
        -> expected<toml::table, std::string>
    {
        auto result = load_value<value_t>(table, variable_path);
        if (not result) {
            return unexpected(result.error());
        }
        output = *result;
        return table;
    };
}

/**
 * \brief Load a native type
 *
 * \param table             the table to load from
 * \param variable_path     the toml path to the value to load
 * \param default_val       the default value to use on failure
 *
 * \return The loaded value, or default value when unsuccsessful
 */
template<value value_t>
value_t load_value_or_else(toml::table const & table,
                           std::string const & variable_path,
                           value_t const & default_val)
{
    auto result = load_value<value_t>(table, variable_path);
    if (not result) {
        return default_val;
    }
    return *result;
}

/**
 * \brief Load a native type
 *
 * \param variable_path     the toml path to the value to load
 * \param output            a reference to write the value to
 * \param default_val       the default value to use on failure
 *
 * \return a function that takes a table and returns a table, such that output
 *         is written to by the loaded value, or by default_val when
 *         unsuccessful.
 */
template<value value_t>
auto load_or_else(std::string const & variable_path,
                  value_t & output,
                  value_t const & default_val)
{
    return [&variable_path, &output, default_val]
           (toml::table const & table)
    {
        output = load_value_or_else(table, variable_path, default_val);
        return table;
    };
}

template<typename range_t>
concept opaque_output_range = (
    std::ranges::output_range<range_t, std::ranges::range_value_t<range_t>> and
    value<std::ranges::range_value_t<range_t>>);

template<typename range_t, typename value_t>
concept output_range = std::ranges::output_range<range_t, value_t> and
                       value<value_t>;

/**
 * \brief Load an array of native types
 *
 * \param table             the table with the array to load
 * \param variable_path     the toml path to the array
 * \param into_array        where to write values to
 *
 * \return the iterator to the next unwritten element
 */
template<opaque_output_range range_t>
expected<std::ranges::iterator_t<range_t>, std::string>
load_array(toml::table const & table,
           std::string const & variable_path,
           range_t && array)
{
    namespace ranges = std::ranges;
    auto result = validate_variable(table, variable_path);
    if (not result) {
        return unexpected(result.error());
    }
    auto node = table.at_path(variable_path);
    if (not node.is_array()) {
        std::string const description = variable_path + " must be an array"s;
        return unexpected{ description };
    }
    auto arr = *node.as_array();
    using value_t = ranges::range_value_t<range_t>;
    // TODO: make this work for non-native types
    // if (not arr.is_homogeneous(node_type_v<value_t>)) {
    //     std::string const description =
    //         "all values in "s + variable_path + " must be homegeneous"s;
    //     return unexpected{ description };
    // }
    if (arr.size() > ranges::size(array)) {
        std::string const description =
            variable_path + " can have at most "s +
            std::to_string(ranges::size(array)) +
            " items, but it has "s + std::to_string(arr.size());
        return unexpected{ description };
    }

    using inserted_t = toml::inserted_type_of<value_t>;
    auto * it = ranges::begin(array);
    arr.for_each([&it](inserted_t const & value) {
        *it++ = static_cast<value_t>(value.get());
    });
    return it;
}
}
