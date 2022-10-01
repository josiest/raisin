#pragma once
#include "raisin/future.hpp"
#include <functional>

// data types
#include <string>

// resource handles
#include <optional>

// deserialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
#include <filesystem>

// type constraints
#include <concepts>
#include <type_traits>
#include <typeinfo>

#include <functional>

using namespace std::string_literals;

namespace raisin {

template<typename value_t>
concept native_type = (std::is_arithmetic_v<value_t> or
                         std::convertible_to<std::string, value_t>);

// node type for non-readables is none
template<typename value_t>
struct toml_node_type :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::none> {};

// node type for bools
template<>
struct toml_node_type<bool> :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::boolean> {};

// node type for integers
template<std::integral value_t>
    requires (not std::same_as<value_t, bool>) // to avoid ambiguity
struct toml_node_type<value_t> :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::integer> {};

// node type for floating points
template<std::floating_point value_t>
struct toml_node_type<value_t> :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::floating_point> {};

// node type for strings
template<typename value_t>
    requires (std::convertible_to<std::string, value_t> and
              (not std::is_arithmetic_v<value_t>))
struct toml_node_type<value_t> :
    public std::integral_constant<toml::node_type,
                                  toml::node_type::string> {};

template<native_type value_t>
toml::node_type constexpr toml_node_type_v = toml_node_type<value_t>::value;

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
        return unexpected("Expecting config at "s + config_path +
                              ", but the file doesn't exist"s);
    }

    toml::parse_result table_result = toml::parse_file(config_path);
    if (not table_result) {
        return unexpected(std::string(table_result.error().description()));
    }
    return table_result.table();
}

expected<toml::table, std::string>
validate_variable(toml::table const & table, std::string const & variable_path)
{
    if (not table.at_path(variable_path)) {
        return unexpected("Expected the variable "s + variable_path + " to "s +
                          "exist, but it doesn't"s);
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
        return unexpected("Expecting "s + variable_path + " to be a table"s +
                          ", but it wasn't"s);
    }
    return *subtable;
}

/**
 * \brief Load a native value
 *
 * \param table             the table to load data from
 * \param variable_path     the toml path to the variable to load
 *
 * \return the loaded value, or a descriptive error message on failure
 */
template<native_type value_t>
expected<value_t, std::string>
load_value(toml::table const & table, std::string const & variable_path)
{
    auto table_result = validate_variable(table, variable_path);
    if (not table_result) {
        return unexpected(table_result.error());
    }

    std::optional<value_t> value_result =
        table_result->at_path(variable_path).value<value_t>();
    if (not value_result) {
        return unexpected("Expecting "s + variable_path + " to have type "s +
                          typeid(value_t).name() + " but it doesn't"s);
    }
    return *value_result;
}

/**
 * \brief Load a native_type
 *
 * \param variable_path     the toml path to the variable to load
 * \param output            where to write the loaded value to
 *
 * \return a function taking a toml::table and returning an expected table
 *         result, such that the target value is written to val when loading
 *         is successful.
 */
template<native_type value_t>
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
template<native_type value_t>
value_t load_value_or_else(toml::table const & table,
                           std::string const & variable_path,
                           value_t default_val)
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
template<native_type value_t>
auto load_or_else(std::string const & variable_path,
                  value_t & output,
                  value_t default_val)
{
    return [&variable_path, &output, default_val]
           (toml::table const & table)
    {
        output = load_value_or_else(table, variable_path, default_val);
        return table;
    };
}

/**
 * \breif Load an array of native types
 *
 * \param table             the table with the array to load
 * \param variable_path     the toml path to the array
 * \param to_array          where to write values to
 */
template<native_type value_t,
         std::weakly_incrementable output_to_array>

expected<toml::table, std::string>
load_array(toml::table const & table, std::string const & variable_path,
           output_to_array to_array)
{
    auto result = validate_variable(table, variable_path);
    if (not result) {
        return unexpected(result.error());
    }
    auto node = table.at_path(variable_path);
    if (not node.is_array()) {
        return unexpected(variable_path + " must be an array"s);
    }
    auto arr = *node.as_array();
    if (not arr.is_homogeneous(toml_node_type_v<value_t>)) {
        return unexpected("all values in " + variable_path + " "s +
                          "must be homegeneous"s);
    }
    using inserted_t = toml::inserted_type_of<value_t>;
    arr.for_each([&to_array](inserted_t const & value) {
        *to_array++ = static_cast<value_t>(value.get());
    });
    return table;
}

/**
 * \brief Load an array of arithemtic or string-type values
 *
 * \param variable_path     the toml variable path to load from
 * \param to_array          where to write values to
 *
 * \return A function that takes a toml::table and writes values
 *         specified at variable_path to the to_array output
 *
 */
template<native_type value_t,
         std::weakly_incrementable output_to_array>
   requires (not native_type<output_to_array>)

auto load(std::string const & variable_path,
          output_to_array to_array)
{
    using namespace std::placeholders;
    return std::bind(load_array, _2, variable_path, _3, to_array);
}
}
