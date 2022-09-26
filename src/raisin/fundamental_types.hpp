#pragma once
#include "raisin/future.hpp"

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

using namespace std::string_literals;

namespace raisin {

/**
 * \brief Determine if a toml::table has an attribute
 * \param name the attribute to query
 * \return a function taking a toml::table and returning an expected result
 */
auto has_attribute(std::string const & name)
{
    return [&name](toml::table const & table)
        -> tl::expected<toml::table, std::string>
    {
        if (not table.at_path(name)) {
            return tl::unexpected("Expecting an attribute named "s + name +
                                  ", but there was none"s);
        }
        return table;
    };
}

/**
 * \brief Get a subtable of a toml::table
 *
 * \param table the table to get a subtable from
 * \param name the name of the subtable
 *
 * \return An expected result of the subtable, or the error message on failure
 */
tl::expected<toml::table, std::string>
subtable(toml::table const & table, std::string const & name)
{
    // make sure the table has the subtable attribute name
    auto validate_attribute = has_attribute(name);
    auto result = validate_attribute(table);
    if (not result) { return result; }

    // make sure the subtable is indeed a table
    toml::table const * sub = table.at_path(name).as_table();
    if (not sub) {
        return tl::unexpected("Expecting "s + name + " to be a table"s +
                              ", but it wasn't"s);
    }
    return *sub;
}

/**
 * \brief Load an arithmetic or string-type value
 *
 * \param name the attribute name in the table
 * \param val a reference to store the value in
 *
 * \return a function taking a toml::table and returning an expected table
 *         result, such that the target value is written to val when loading
 *         is successful.
 */
template<typename value_t>
    requires (std::is_arithmetic_v<value_t> or
              std::convertible_to<value_t, std::string>)
auto load(std::string const & name, value_t & val)
{
    return [&name, &val](toml::table const & table)
        -> tl::expected<toml::table, std::string>
    {
        // make sure the attribute exists in the table
        auto validate_attribute = has_attribute(name);
        auto table_result = validate_attribute(table);
        if (not table_result) {
            return table_result;
        }

        // make sure the attribute is a valid type
        std::optional<value_t> value_result =
                table_result->at_path(name).value<value_t>();
        if (not value_result) {
            return tl::unexpected("Expecting "s + name + " to be " +
                                  typeid(val).name() + ", but it wasn't"s);
        }

        val = *value_result;
        return table_result;
    };
}

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
tl::expected<toml::table, std::string>
parse_file(std::string const & config_path)
{
    if (not std::filesystem::exists(config_path)) {
        return tl::unexpected("Expecting config at "s + config_path +
                              ", but the file doesn't exist"s);
    }

    toml::parse_result table_result = toml::parse_file(config_path);
    if (not table_result) {
        return tl::unexpected(std::string(table_result.error().description()));
    }
    return table_result.table();
}

}
