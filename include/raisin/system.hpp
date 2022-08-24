#include <SDL2/SDL.h>
#include <string>
#include <cstdint>

#include <optional>
#include <variant>
#include <unordered_map>

#include <algorithm>
#include <iterator>
#include "raisin/future.hpp"

#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>

namespace raisin::serialization {

using namespace std::string_literals;

inline static std::unordered_map<std::string, std::uint32_t>
const _as_subsystem_flag{
    { "timer", SDL_INIT_TIMER }, { "audio", SDL_INIT_AUDIO },
    { "video", SDL_INIT_VIDEO }, { "joystick", SDL_INIT_JOYSTICK },
    { "haptic", SDL_INIT_HAPTIC },
    { "game-controller", SDL_INIT_GAMECONTROLLER },
    { "events", SDL_INIT_EVENTS }, { "everything", SDL_INIT_EVERYTHING }
};

std::string
inline _strlower(std::string const & str)
{
    auto tolower = [](unsigned char ch) { return std::tolower(ch); };
    return str | views::transform(tolower) | ranges::to<std::string>();
}

inline std::optional<std::uint32_t>
as_subsystem_flag(std::string const & name)
{
    auto const lower_name = _strlower(name);
    if (not _as_subsystem_flag.contains(lower_name)) {
        return std::nullopt;
    }
    return _as_subsystem_flag.at(lower_name);
}

/**
 * Parse a range of subsystem names into an SDL subsystem flag.
 *
 * Parameters:
 *   \param subsystems - the subsystem names to parse
 *   \param invalid_names - where to write invalid names
 *
 * \return the union of the subsystem flags as integers. Any names that aren't
 *         valid subsystems will not be included in the union, but will be
 *         written to invalid_names.
 */
template<ranges::input_range name_reader,
         std::weakly_incrementable name_writer>

std::uint32_t
parse_subsystem_flags(name_reader const & subsystems,
                      name_writer invalid_names)
{
    // make sure all names are lower-case
    auto lower_names = subsystems | views::transform(_strlower)
                                  | ranges::to<std::vector>;

    // write down any subsystem names that are invalid
    auto is_not_subsystem = [](std::string const & name) {
        return not _as_subsystem_flag.contains(name);
    };
    ranges::copy_if(lower_names, invalid_names, is_not_subsystem);

    // transform valid names into flags and join
    auto is_subsystem = [](std::string const & name) {
        return _as_subsystem_flag.contains(name);
    };
    auto as_flag = [](std::string const & name) {
        return _as_subsystem_flag.at(name);
    };
    auto join = [](std::uint32_t sum, std::uint32_t x) { return sum | x; };
    return ranges::accumulate(lower_names | views::filter(is_subsystem)
                                          | views::transform(as_flag),
                               0u, join);
}

/**
 * Load SDL Subsystem names from a config file into a writable iterator.
 *
 * Parameters:
 *   \param config_path - the name of the path to load
 *   \param subsystems - the subsystem names to write to
 *
 * \return the number of names written on succesfully parsing the config file.
 *         Otherwise, return a parse
 */
template<std::weakly_incrementable name_writer>
std::variant<std::size_t, std::string>

load_subsystem_names(std::string const & config_path,
                     name_writer subsystems)
{
    // read the config file, return any parsing errors
    toml::parse_result result = toml::parse_file(config_path);
    if (not result) {
        return std::string(result.error().description());
    }
    auto table = std::move(result).table();

    // if subsystem-flags aren't specified then there are no susbystems written
    auto flag_node = table["system"]["subsystems"];
    if (not flag_node) {
        return 0u;
    }
    // system.subsystems must be an array of strings
    if (not flag_node.is_array()) {
        return "system.subsystems must be an array"s;
    }
    auto flags = *flag_node.as_array();
    if (not flags.is_homogeneous(toml::node_type::string)) {
        return "all subsytem names in system.subsystems must be strings"s;
    }
    // copy the flags as strings into the subsystem names
    flags.for_each([&subsystems](toml::value<std::string> const & name) {
        *subsystems = name.get();
        ++subsystems;
    });
    return flags.size();
}
}
