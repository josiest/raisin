#include <SDL2/SDL.h>
#include <string>
#include <cstdint>

#include <optional>
#include <unordered_map>

#include <algorithm>
#include <iterator>
#include "raisin/future.hpp"

namespace raisin::serialization {

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
 *   \param not_subsystems - where to write invalid names
 *
 * \return the subsystem flags as an integer if all names were valid.
 *         Otherwise write all invalid names to not_subsystems and return
 *         nullopt.
 */
template<ranges::input_range name_reader,
         std::weakly_incrementable name_writer>

std::optional<std::uint32_t>
parse_subsystem_flags(name_reader const & subsystems,
                      name_writer not_subsystems)
{
    // make sure all names are lower-case
    auto lower_names = subsystems | views::transform(_strlower)
                                  | ranges::to<std::vector<std::string>>;

    // if any subsystem names are invalid
    //   write them down and return nothing
    auto is_not_subsystem = [](std::string const & name) {
        return not _as_subsystem_flag.contains(name);
    };
    if (ranges::any_of(lower_names, is_not_subsystem)) {
        ranges::copy_if(lower_names, not_subsystems, is_not_subsystem);
        return std::nullopt;
    }

    // otherwise transform into flags and join
    auto as_flag = [](std::string const & name) {
        return _as_subsystem_flag.at(name);
    };
    auto join = [](std::uint32_t sum, std::uint32_t x) { return sum | x; };
    return ranges::accumulate(lower_names
                            | views::transform(as_flag), 0u, join);
}

}
