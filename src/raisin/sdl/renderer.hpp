#pragma once
#include "raisin/future.hpp"
#include "raisin/flags.hpp"

// frameworks
#include <SDL2/SDL.h>

// data types and structures
#include <string>
#include <cstdint>
#include <unordered_map>

// serialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>

namespace raisin::sdl {

/**
 * Initialize SDL with the subsystems defined in a toml config file.
 *
 * \param variable_path     the toml path to the variable
 * \param flag_output       where to write the flag to
 * \param invalid_names     a place to write any invalid names to
 *
 * \return A function that takes a toml::table and returns an expected table
 *         result such that the resulting flag is written to flag_output.
 *
 * \note Any names that aren't valid subsystems will not be included in the
 *       union, but will be written to invalid_names.
 *
 * Acceptable flag names:
 * - software
 * - accelerated
 * - present-vsync
 * - target-texture
 */
template<std::size_t max_flags = limits::max_flags,
         std::weakly_incrementable name_output>
requires std::indirectly_writable<name_output, std::string>

expected<std::uint32_t, std::string>
load_renderer_flags(toml::table const & table,
                    std::string const & variable_path,
                    name_output into_invalid_names)
{
    static std::unordered_map<std::string, std::uint32_t>
    const as_renderer_flag{
        { "software",       SDL_RENDERER_SOFTWARE },
        { "accelerated",    SDL_RENDERER_ACCELERATED },
        { "present-vsync",  SDL_RENDERER_PRESENTVSYNC },
        { "target-texture", SDL_RENDERER_TARGETTEXTURE },
    };

    return load_flags<max_flags>(table, variable_path,
                                 as_renderer_flag, into_invalid_names);
}

/**
 * \brief Load SDL renderer flags
 *
 * \param variable_path     the toml path to the flags
 * \param flag_output       a reference to write the flags to
 * \param invalid_names     to write any invalid names to
 *
 * \return a function taking a table and returning an expected table result,
 *         such that the flags are written to output when loading succeeds.
 */
template<std::size_t max_flags = limits::max_flags,
         std::weakly_incrementable name_output>
requires std::indirectly_writable<name_output, std::string>

auto load_renderer_flags_into(std::string const & variable_path,
                              std::uint32_t & flag_output,
                              name_output into_invalid_names)
{
    return _load_flags(load_renderer_flags<max_flags, name_output>,
                       variable_path, flag_output, into_invalid_names);
}

/**
 * Create an SDL_Renderer from a toml::table of parameters
 *
 * \param variable_path     the toml path to the renderer parameters
 * \param window            to render to
 * \param renderer_output   where to write the renderer
 * \param invalid_flags     a place to write any invalid renderer flag names
 *
 * \return a function taking a toml::table and returns an expected table result
 *         such that the created renderer is written to renderer_output
 *
 * \note All invalid renderer flag names will be written to invalid_flags
 *       See documentation for load_renderer_flags for a list of valid
 *       renderer flag names
 *
 * toml parameters:
 *
 *  list<string> flags  REQUIRED    renderer flags to use when creating the
 *                                  renderer
 *
 *  int driver_index    OPTIONAL    the index of the video driver
 *                                  defaults to the first available (-1)
 */
template<std::size_t max_flags = limits::max_flags,
         std::weakly_incrementable name_output>
requires std::indirectly_writable<name_output, std::string>
auto load_renderer(std::string const & variable_path,
                   SDL_Window * window,
                   SDL_Renderer * & renderer_output,
                   name_output into_invalid_names)
{
    return [&renderer_output, window, &variable_path, into_invalid_names]
           (toml::table const & table)
        -> expected<toml::table, std::string>
    {
        std::uint32_t flags;
        int driver_index;
        auto result = subtable(table, variable_path)
            .and_then(load_renderer_flags_into<max_flags>(
                "flags", flags, into_invalid_names))
            .map(load_or_else("driver_index", driver_index, -1));

        if (not result) { return result; }

        renderer_output = SDL_CreateRenderer(window, driver_index, flags);
        if (not renderer_output) {
            return unexpected(SDL_GetError());
        }
        return table;
    };
}
}
