#include "raisin/system.hpp"
#include "raisin/window.hpp"
#include "raisin/future.hpp"
#include <SDL2/SDL.h>

#include <string>
#include <vector>
#include <variant>

#include <iostream>
#include <iterator>

int main()
{
    using namespace raisin;
    std::string const config_path = "../examples/assets/config.toml";

    // load the subsystem flags from file
    std::vector<std::string> invalid_subsystems;
    auto init_result = init_sdl_from_config(
            config_path,
            std::back_inserter(invalid_subsystems));

    for (auto const & name : invalid_subsystems) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
                    "No subsystem named \"%s\", skipping",
                    name.c_str());
    }
    // some other error on SDL side occured when initializing
    if (not init_result) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
                        "Unable to initialize SDL: %s",
                        init_result.error().c_str());
        return EXIT_FAILURE;
    }

    std::vector<std::string> invalid_window_flags;
    auto window = make_window_from_config(
            config_path,
            std::back_inserter(invalid_window_flags));
    if (not window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO,
                        "Unable to create a window: %s",
                        window.error().c_str());
    }
    for (auto const & name : invalid_window_flags) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
                    "No window flag named \"%s\", skipping",
                    name.c_str());
    }
    SDL_Delay(2000);

    SDL_DestroyWindow(*window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
