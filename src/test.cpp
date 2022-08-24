#include "raisin/system.hpp"
#include "raisin/future.hpp"
#include <SDL2/SDL.h>

#include <string>
#include <vector>
#include <variant>

#include <iostream>
#include <iterator>

int main()
{
    using namespace raisin::serialization;

    std::vector<std::string> subsystems;
    auto result = load_subsystem_names(
            "../examples/assets/config.toml",
            std::back_inserter(subsystems));

    if (std::holds_alternative<std::string>(result)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
                        "Couldn't parse system config: %s",
                        std::get<std::string>(result).c_str());
        return EXIT_FAILURE;
    }

    // transform ubsystem names into an int flag
    // keep track of any names that were invalid
    std::vector<std::string> invalid_names;
    invalid_names.reserve(subsystems.size());

    std::uint32_t const flags = parse_subsystem_flags(
            subsystems,
            std::back_inserter(invalid_names));

    // log any names that were invalid
    auto log_invalid_name = [](std::string const & name) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
                    "No subsystem named %s, skipping",
                    name.c_str());
    };
    ranges::for_each(invalid_names, log_invalid_name);

    // some other error on SDL side occured when initializing
    if (SDL_Init(flags) != 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
                        "Unable to initialize SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Log("Initialized SDL succesfully");
    SDL_Quit();

    return EXIT_SUCCESS;
}
