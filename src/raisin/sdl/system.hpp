#pragma once
#include "raisin/future.hpp"
#include "raisin/flags.hpp"

// low-level frameworks
#include <SDL2/SDL.h>

// data types
#include <string>
#include <cstdint>

// data structures/resource handles
#include <optional>
#include <unordered_map>

// algorithms
#include <algorithm>
#include <functional>

// type constraints
#include <iterator>
#include <concepts>

// serialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>

namespace raisin::sdl {

/**
 * \brief Load SDL subsystem flags.
 *
 * \param table             the table with the flags
 * \param variable_path     the toml path to the variable to load
 * \param invalid_names     a place to write any invalid names to
 *
 * \return the union of the subsystem flags as integers if parsing was
 *         successful.
 *
 * \note Any names that aren't valid subsystems will not be included in the
 *       union, but will be written to invalid_names.
 */
template<std::unsigned_integral flag_t,
         output_range<std::string> name_output>

expected<flag_t, std::string>
load_subsystem_flags(toml::table const & table,
                     std::string const & variable_path,
                     name_output && invalid_names)
{

    static std::unordered_map<std::string, std::uint32_t>
    const as_subsystem_flag{
        { "timer",              SDL_INIT_TIMER },
        { "audio",              SDL_INIT_AUDIO },
        { "video",              SDL_INIT_VIDEO },
        { "joystick",           SDL_INIT_JOYSTICK },
        { "haptic",             SDL_INIT_HAPTIC },
        { "game-controller",    SDL_INIT_GAMECONTROLLER },
        { "events",             SDL_INIT_EVENTS },
        { "everything",         SDL_INIT_EVERYTHING }
    };

    return _flags_from_map(as_subsystem_flag, table, variable_path,
                           std::forward<name_output>(invalid_names));
}

/**
 * \brief Load SDL subsystem flags
 *
 * \param variable_path         the toml path to the flags
 * \param flag_output           a reference to write the flags to
 * \param invalid_names    to write any invalid names to
 *
 * \return a function taking a table and returning an expected table result,
 *         such that the flags are written to output when loading succeeds.
 */
template<std::unsigned_integral flag_t,
         output_range<std::string> name_output>

auto load_subsystem_flags_into(std::string const & variable_path,
                               flag_t & flag_output,
                               name_output && invalid_names)
{
    return _load_flags(load_subsystem_flags<flag_t, name_output>,
                       variable_path, flag_output,
                       std::forward<name_output>(invalid_names));
}

/**
 * \brief Initialize SDL with the subsystems defined in a toml config file.
 *
 * \param config_path - the path to the toml config file
 * \param invalid_names - a place to write any invalid names to
 *
 * \note Any names that aren't valid subsystems will be skipped over and written
 *       to invalid_names.
 */
template<output_range<std::string> name_output>
auto init_sdl(std::string const & variable_path,
              name_output && invalid_names)
{
    return [&variable_path, &invalid_names]
           (toml::table const & table)
        -> expected<toml::table, std::string>
    {
        std::uint32_t flags;
        auto result = subtable(table, variable_path)
            .and_then( load_subsystem_flags_into(
                "subsystems", flags,
                std::forward<name_output>(invalid_names)));

        if (not result) {
            return result;
        }
        if (SDL_Init(flags) != 0) {
            return unexpected(SDL_GetError());
        }
        return table;
    };
}
}
