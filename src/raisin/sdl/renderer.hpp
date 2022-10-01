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
template<std::unsigned_integral flag_t,
         std::weakly_incrementable name_output_t>

expected<flag_t, std::string>
load_renderer_flags(toml::table const & table,
                    std::string const & variable_path,
                    name_output_t into_invalid_names)
{
    static std::unordered_map<std::string, std::uint32_t>
    const _as_renderer_flag{
        { "software", SDL_RENDERER_SOFTWARE },
        { "accelerated", SDL_RENDERER_ACCELERATED },
        { "present-vsync", SDL_RENDERER_PRESENTVSYNC },
        { "target-texture", SDL_RENDERER_TARGETTEXTURE },
    };

    return _flags_from_map(table, _as_renderer_flag,
                           variable_path, into_invalid_names);
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
template<std::weakly_incrementable name_writer>
auto load_renderer(std::string const & variable_path,
                   SDL_Window * window,
                   SDL_Renderer * & renderer_output,
                   name_writer invalid_flag_names)
{
    return [&variable_path, &renderer_output, window, invalid_flag_names]
           (toml::table const & table)
        -> expected<toml::table, std::string>
    {
        int driver_index;
        auto result = subtable(table, variable_path)
            .map(load_or_else("driver_index", driver_index, -1));
        if (not result) { return result; }

        // TODO: add monadic overload
        using namespace std::string_literals;
        auto flag_result = load_renderer_flags<std::uint32_t>(
                table, variable_path + ".flags"s, invalid_flag_names);
        if (not flag_result) {
            return unexpected(flag_result.error());
        }
        std::uint32_t const flags = *flag_result;

        renderer_output = SDL_CreateRenderer(window, driver_index, flags);
        if (not renderer_output) {
            return unexpected(SDL_GetError());
        }
        return table;
    };
}
}
