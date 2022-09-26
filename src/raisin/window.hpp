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

namespace raisin {

/**
 * Parse SDL window flags from a toml::table
 *
 * \param variable_path     the toml path to the window parameters
 * \param flag_output       to write the parsed flag out to
 * \param invalid_names     a place to write any invalid names to
 *
 * \return A function that takes a toml::table and returns an expected table
 *         result such that the parsed flag is written to flag_output.
 *
 * \note Any names that aren't valid subsystems will not be included in the
 *       union, but will be written to invalid_names.
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
template<std::weakly_incrementable name_writer>
auto load_window_flags(std::string const & variable_path,
                       std::uint32_t & flag_output,
                       name_writer invalid_names)
{
    static std::unordered_map<std::string, std::uint32_t>
    const _as_window_flag{
        { "fullscreen", SDL_WINDOW_FULLSCREEN },
        { "fullscreen-desktop", SDL_WINDOW_FULLSCREEN_DESKTOP },
        { "opengl", SDL_WINDOW_OPENGL }, { "vulkan", SDL_WINDOW_VULKAN },
        { "metal", SDL_WINDOW_METAL }, { "hidden", SDL_WINDOW_HIDDEN },
        { "borderless", SDL_WINDOW_BORDERLESS },
        { "resizable", SDL_WINDOW_RESIZABLE },
        { "minimized", SDL_WINDOW_MINIMIZED },
        { "maximized", SDL_WINDOW_MAXIMIZED },
        { "input-grabbed", SDL_WINDOW_INPUT_GRABBED },
        { "allow-high-dpi", SDL_WINDOW_ALLOW_HIGHDPI },
        { "shown", SDL_WINDOW_SHOWN }
    };

    return _flags_from_map(_as_window_flag, variable_path,
                           flag_output, invalid_names);
}

/**
 * \brief Create an SDL_Window from toml::table of window parameters.
 *
 * \param variable_path     the toml path to the table of window parameters
 * \param window_output     a reference to write the window to
 * \param invalid_flags     a place to write any invalid window flag names
 *
 * \return a function taking a toml::table and returns an expected table result
 *         such that the created window is written to window_output
 *
 * \note All invalid window flag names will be written to invalid_flags.
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
 *  list<string> flags  OPTIONAL    the flags to use creating the window
 */
template<std::weakly_incrementable name_writer>
auto load_window(std::string const & variable_path,
                 SDL_Window * & window_output,
                 name_writer invalid_flags)
{
    return [&variable_path, &window_output, invalid_flags]
           (toml::table const & table)
        -> expected<toml::table, std::string>
    {
        std::string title{};
        int x{};
        int y{};
        std::uint32_t width{};
        std::uint32_t height{};
        std::uint32_t flags{};

        auto result = subtable(table, "window")
            .and_then(load("title", title))
            .and_then(load("width", width))
            .and_then(load("height", height))
            .and_then(load_window_flags("flags", flags, invalid_flags))
            .map(load_or_else("x", x,
                              static_cast<int>(SDL_WINDOWPOS_UNDEFINED)))
            .map(load_or_else("y", y,
                              static_cast<int>(SDL_WINDOWPOS_UNDEFINED)));
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
