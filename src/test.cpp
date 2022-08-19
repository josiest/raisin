#include <SDL2/SDL.h>

#include <string>
#include <cstdint>

#include <optional>
#include <unordered_map>

#include <algorithm>
#include "raisin/future.hpp"

namespace sdl::serialization {

static std::unordered_map<std::string, std::uint32_t> const _as_subsystem_flag{
    { "timer", SDL_INIT_TIMER }, { "audio", SDL_INIT_AUDIO },
    { "video", SDL_INIT_VIDEO }, { "joystick", SDL_INIT_JOYSTICK },
    { "haptic", SDL_INIT_HAPTIC },
    { "game-controller", SDL_INIT_GAMECONTROLLER },
    { "events", SDL_INIT_EVENTS }, { "everything", SDL_INIT_EVERYTHING }
};

std::optional<std::uint32_t>
as_subsystem_flag(std::string const & name)
{
    using namespace sdl;
    auto to_lower = [](unsigned char ch) { return std::tolower(ch); };
    auto const lower_name = name | views::transform(to_lower)
                                 | ranges::to<std::string>();

    if (not _as_subsystem_flag.contains(lower_name)) {
        return std::nullopt;
    }
    return _as_subsystem_flag.at(lower_name);
}
}

int main()
{
    using namespace sdl::serialization;
    std::string const name{"video"};
    SDL_Log("Subsystem flag for \"%s\" is %d",
            name.c_str(), *as_subsystem_flag(name));
}
