#include <SDL2/SDL.h>
#include <string>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <algorithm>

namespace sdl::serialization {

std::optional<std::uint32_t>
as_subsystem_flag(std::string const & name)
{
    static std::unordered_map<std::string, std::uint32_t> const as_flag{
        { "timer", SDL_INIT_TIMER }, { "audio", SDL_INIT_AUDIO },
        { "video", SDL_INIT_VIDEO }, { "joystick", SDL_INIT_JOYSTICK },
        { "haptic", SDL_INIT_HAPTIC },
        { "game-controller", SDL_INIT_GAMECONTROLLER },
        { "events", SDL_INIT_EVENTS }, { "everything", SDL_INIT_EVERYTHING }
    };

    std::string lower_name{name};
    std::transform(name.begin(), name.end(), lower_name.begin(),
                   [](unsigned char ch) { return std::tolower(ch); });

    if (not as_flag.contains(name)) {
        return std::nullopt;
    }
    return as_flag.at(name);
}
}

int main()
{
    using namespace sdl::serialization;
    std::string const name{"video"};
    SDL_Log("Subsystem flag for \"%s\" is %d",
            name.c_str(), *as_subsystem_flag(name));
}
