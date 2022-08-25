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

    // load the subsystem flags from file
    std::vector<std::string> invalid_names;
    auto subsystems_result = load_subsystems_from_config(
            "../examples/assets/config.toml",
            std::back_inserter(invalid_names));

    // log any subsystem names that are invalid
    if (not subsystems_result) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
                        "Couldn't parse system config: %s",
                        subsystems_result.error().c_str());
        return EXIT_FAILURE;
    }
    for (auto const & name : invalid_names) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
                    "No subsystem named %s, skipping",
                    name.c_str());
    }
    // some other error on SDL side occured when initializing
    if (SDL_Init(*subsystems_result) != 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
                        "Unable to initialize SDL: %s",
                        SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Log("Initialized SDL succesfully");
    SDL_Quit();

    return EXIT_SUCCESS;
}
