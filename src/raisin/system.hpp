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

using namespace std::string_literals;

/**
 * \brief Load SDL subsystem flags from a config file.
 *
 * \param config_path - the path to the toml config file
 * \param invalid_names - a place to write any invalid names to
 *
 * \return the union of the subsystem flags as integers if parsing was
 *         successful.
 *
 * \note Any names that aren't valid subsystems will not be included in the
 *       union, but will be written to invalid_names.
 */
template<std::weakly_incrementable name_writer>
expected<std::uint32_t, std::string>
load_subsystem_flags(std::string const & config_path,
                     name_writer invalid_names)
{

    static std::unordered_map<std::string, std::uint32_t>
    const _as_subsystem_flag{
        { "timer", SDL_INIT_TIMER }, { "audio", SDL_INIT_AUDIO },
        { "video", SDL_INIT_VIDEO }, { "joystick", SDL_INIT_JOYSTICK },
        { "haptic", SDL_INIT_HAPTIC },
        { "game-controller", SDL_INIT_GAMECONTROLLER },
        { "events", SDL_INIT_EVENTS }, { "everything", SDL_INIT_EVERYTHING }
    };

    std::vector<std::string> subsystems;
    auto subsystems_result = load_flag_names(config_path, "system.subsystems",
                                             std::back_inserter(subsystems));

    if (not subsystems_result) {
        return unexpected(subsystems_result.error());
    }
    return parse_flags(subsystems,
        [](auto const & name) { return _as_subsystem_flag.contains(name); },
        [](auto const & name) { return _as_subsystem_flag.at(name); },
        invalid_names);
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
template<std::weakly_incrementable name_writer>
expected<bool, std::string>
init_sdl_from_config(std::string const & config_path,
                     name_writer invalid_names)
{
    auto flags = load_subsystem_flags(config_path, invalid_names);
    if (not flags) {
        return unexpected(flags.error().c_str());
    }
    if (SDL_Init(*flags) != 0) {
        return unexpected(SDL_GetError());
    }
    return true;
}
}
