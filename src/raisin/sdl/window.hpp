#pragma once
#include "raisin/future.hpp"
#include "raisin/flags.hpp"

// low-level frameworks
#include <SDL2/SDL.h>

// data types
#include <string>
#include <cstdint>

// data structures
#include <unordered_map>

// serialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>

namespace raisin::sdl {

/**
 * Parse SDL window flags from a toml::table
 *
 * \param table                 the table with the window parameters
 * \param variable_path         the toml path to the window flags
 * \param invalid_names         a place to write any invalid names to
 *
 * \return The table when loading succeeds, otherwise a descriptive error
 *         message.
 *
 * \note Any names that aren't valid subsystems will not be included in the
 *       union, but will be written invalid_names.
 *
 * Acceptable flag names:
 * - fullscreen
 * - fullscreen-desktop
 * - opengl
 * - vulkan
 * - metal
 * - hidden
 * - borderless
 * - resizable
 * - minimized
 * - maximized
 * - input-grabbed
 * - allow-high-dpi
 * - shown
 */
template<std::size_t max_flags = limits::max_flags,
         std::weakly_incrementable name_output>
requires std::indirectly_writable<name_output, std::string>

expected<std::uint32_t, std::string>
load_window_flags(toml::table const & table,
                  std::string const & variable_path,
                  name_output into_invalid_names)
{
    static std::unordered_map<std::string, std::uint32_t>
    const as_window_flag{
        { "fullscreen",         SDL_WINDOW_FULLSCREEN },
        { "fullscreen-desktop", SDL_WINDOW_FULLSCREEN_DESKTOP },
        { "opengl",             SDL_WINDOW_OPENGL },
        { "vulkan",             SDL_WINDOW_VULKAN },
        { "metal",              SDL_WINDOW_METAL },
        { "hidden",             SDL_WINDOW_HIDDEN },
        { "borderless",         SDL_WINDOW_BORDERLESS },
        { "resizable",          SDL_WINDOW_RESIZABLE },
        { "minimized",          SDL_WINDOW_MINIMIZED },
        { "maximized",          SDL_WINDOW_MAXIMIZED },
        { "input-grabbed",      SDL_WINDOW_INPUT_GRABBED },
        { "allow-high-dpi",     SDL_WINDOW_ALLOW_HIGHDPI },
        { "shown",              SDL_WINDOW_SHOWN }
    };

    return load_flags<max_flags>(table, variable_path,
                                 as_window_flag, into_invalid_names);
}

/**
 * \brief Load SDL window flags
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

auto load_window_flags_into(std::string const & variable_path,
                            std::uint32_t & flag_output,
                            name_output into_invalid_names)
{
    return _load_flags(load_window_flags<max_flags, name_output>,
                       variable_path, flag_output, into_invalid_names);
}

/**
 * \brief Create an SDL_Window from toml::table of window parameters.
 *
 * \param variable_path         the toml path to the table of window parameters
 * \param window_output         a reference to write the window to
 * \param invalid_names    a place to write any invalid window flag names
 *
 * \return a function taking a toml::table and returns an expected table result
 *         such that the created window is written to window_output
 *
 * \note All invalid window flag names will be written invalid_names.
 *       See documentation for load_window_flags for a list of valid window
 *       flags.
 *
 * toml parameters:
 *
 *  string title        REQUIRED
 *  int width           REQUIRED
 *  int height          REQUIRED
 *
 *  int x               OPTIONAL    defaults to SDL_WINDOWPOS_UNDEFINED
 *  int y               OPTIONAL    defaults to SDL_WINDOWPOS_UNDEFINED
 *  array<string> flags OPTIONAL    the flags to use creating the window
 */
template<std::size_t max_flags = limits::max_flags,
         std::weakly_incrementable name_output>
requires std::indirectly_writable<name_output, std::string>
auto load_window(std::string const & variable_path,
                 SDL_Window * & window_output,
                 name_output into_invalid_names)
{
    return [&variable_path, &window_output, into_invalid_names]
           (toml::table const & table)
        -> expected<toml::table, std::string>
    {
        std::string title;
        int x, y;
        std::uint32_t width, height, flags;
        int constexpr anywhere = static_cast<int>(SDL_WINDOWPOS_UNDEFINED);

        auto result = subtable(table, variable_path)
            .and_then(load("title", title))
            .and_then(load("width", width))
            .and_then(load("height", height))
            .and_then(load_window_flags_into<max_flags>(
                "flags", flags, into_invalid_names))
            .map(load_or_else("x", x, anywhere))
            .map(load_or_else("y", y, anywhere));

        if (not result) { return result; }

        window_output = SDL_CreateWindow(
                title.c_str(), x, y, width, height, flags);
        if (not window_output) {
            return unexpected(SDL_GetError());
        }
        return table;
    };
}
}
