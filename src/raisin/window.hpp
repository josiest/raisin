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
#include <iterator>

// serialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>

namespace raisin {

/**
 * Initialize SDL with the subsystems defined in a toml config file.
 *
 * \param config_path - the path to the toml config file
 * \param invalid_names - a place to write any invalid names to
 *
 * \return the union of the subsystem flags as integers if parsing was
 *         successful.
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
expected<std::uint32_t, std::string>
load_window_flags(std::string const & config_path,
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

    std::vector<std::string> flags;
    auto flag_result = load_flag_names(config_path, "window.flags",
                                       std::back_inserter(flags));

    if (not flag_result) {
        return unexpected(flag_result.error());
    }
    return parse_flags(flags,
                       [](auto const & name) { return _as_window_flag.contains(name); },
                       [](auto const & name) { return _as_window_flag.at(name); },
                       invalid_names);
}

/**
 * Create an SDL_Window from a config file.
 *
 * \param config_path - the config filepath
 * \param invalid_flag_names - a place to write any invalid window flag names to
 */
template<std::weakly_incrementable name_writer>
expected<SDL_Window *, std::string>
make_window_from_config(std::string const & config_path,
                        name_writer invalid_flag_names)
{
    // parse window flags
    auto flags = load_window_flags(config_path, invalid_flag_names);
    if (not flags) {
        return unexpected(flags.error());
    }

    // read config file
    toml::parse_result table_result = toml::parse_file(config_path);
    if (not table_result) {
        return unexpected(std::string(table_result.error().description()));
    }
    auto table = std::move(table_result).table();

    // can't make a window if no window parameters were specified
    if (not table["window"]) {
        return unexpected("config at "s + config_path +
                              "has no window settings"s);
    }
    if (not table["window"].is_table()) {
        return unexpected("window config settings must be a table!"s);
    }
    toml::table window = *table["window"].as_table();

    // parse title
    if (not window["title"]) {
        return unexpected("window config must have a title!"s);
    }
    if (not window["title"].is_string()) {
        return unexpected("window.title must be a string!"s);
    }
    auto const title = window["title"].as_string()->get();

    // parse width
    if (not window["width"]) {
        return unexpected("window config must have a width!"s);
    }
    
    if (not window["width"].is_integer()) {
        return unexpected("window.width must be an integer"s);
    }
    std::uint32_t const width = window["width"].as_integer()->get();

    // parse height
    if (not window["height"]) {
        return unexpected("window config must have a height!"s);
    }
    if (not window["height"].is_integer()) {
        return unexpected("window.height must be an integer"s);
    }
    std::uint32_t const height = window["height"].as_integer()->get();

    // parse x
    int x = SDL_WINDOWPOS_UNDEFINED;
    if (window["x"].is_integer()) {
        x = window["x"].as_integer()->get();
    }
    else if (window["x"]) {
        return unexpected("window.x must be an integer"s);
    }

    // parse y
    int y = SDL_WINDOWPOS_UNDEFINED;
    if (window["y"].is_integer()) {
        y = window["x"].as_integer()->get();
    }
    else if (window["y"]) {
        return unexpected("window.y must be an integer"s);
    }

    SDL_Window * sdl_window = SDL_CreateWindow(title.c_str(), x, y,
                                               width, height, *flags);
    if (not sdl_window) {
        return unexpected(SDL_GetError());
    }
    return sdl_window;
}
}
