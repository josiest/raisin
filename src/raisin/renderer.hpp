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
 * - software
 * - accelerated
 * - present-vsync
 * - target-texture
 */
template<std::weakly_incrementable name_writer>
expected<std::uint32_t, std::string>
load_renderer_flags(std::string const & config_path,
                    name_writer invalid_names)
{
    static std::unordered_map<std::string, std::uint32_t>
    const _as_renderer_flag{
        { "software", SDL_RENDERER_SOFTWARE },
        { "accelerated", SDL_RENDERER_ACCELERATED },
        { "present-vsync", SDL_RENDERER_PRESENTVSYNC },
        { "target-texture", SDL_RENDERER_TARGETTEXTURE },
    };

    std::vector<std::string> flags;
    auto flag_result = load_flag_names(config_path, "renderer.flags",
                                       std::back_inserter(flags));

    if (not flag_result) {
        return unexpected(flag_result.error());
    }
    return parse_flags(flags,
        [](auto const & name) { return _as_renderer_flag.contains(name); },
        [](auto const & name) { return _as_renderer_flag.at(name); },
        invalid_names);
}

/**
 * Create an SDL_Renderer from a config file.
 *
 * \param config_path the config file path
 * \param window to render to
 * \param invalid_flag_names a place to write any invalid window flag names to
 */
template<std::weakly_incrementable name_writer>
expected<SDL_Renderer *, std::string>
make_renderer_from_config(std::string const & config_path,
                          SDL_Window * window,
                          name_writer invalid_flag_names)
{
    // read config file
    toml::parse_result table_result = toml::parse_file(config_path);
    if (not table_result) {
        return unexpected(std::string(table_result.error().description()));
    }
    auto table = std::move(table_result).table();

    // fail if no renderer parameters were specified
    if (not table["renderer"]) {
        return unexpected("config at "s + config_path +
                              " has no renderer settings"s);
    }
    if (not table["renderer"].is_table()) {
        return unexpected("renderer config settings must be a table!"s);
    }
    toml::table renderer = *table["renderer"].as_table();

    // parse renderer driver
    int driver_index = -1;
    if (renderer["driver_index"].is_integer()) {
        driver_index = renderer["driver_index"].as_integer()->get();
    }
    else if (renderer["driver_index"]) {
        return unexpected("renderer.driver_index must be an integer"s);
    }

    // parse renderer flags
    auto flags = load_renderer_flags(config_path, invalid_flag_names);
    if (not flags) {
        return unexpected(flags.error());
    }

    auto * sdl_renderer = SDL_CreateRenderer(window, driver_index, *flags);
    if (not sdl_renderer) {
        return unexpected("Couldn't create renderer: "s + SDL_GetError());
    }
    return sdl_renderer;
}
}
