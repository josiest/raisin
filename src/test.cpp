// frameworkds
#include "raisin/raisin.hpp"
#include <SDL2/SDL.h>

// data types
#include <string>

// resource handles
#include <vector>

// algorithms
#include <iterator>

int main()
{
    using namespace raisin;
    std::string const config_path = "../examples/assets/config.toml";

    // load the subsystem flags from file
    std::vector<std::string> invalid_flags;
    auto init_result = init_sdl_from_config(
            config_path,
            std::back_inserter(invalid_flags));
    for (auto const & name : invalid_flags) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
            "No subsystem named \"%s\", skipping",
            name.c_str());
    }
    invalid_flags.clear();

    // some other error on SDL side occured when initializing
    if (not init_result) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
            "Unable to initialize SDL: %s",
            init_result.error().c_str());
        return EXIT_FAILURE;
    }

    // load the window from config settings
    auto window = make_window_from_config(
            config_path,
            std::back_inserter(invalid_flags));
    for (auto const & name : invalid_flags) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
            "No window flag named \"%s\", skipping",
            name.c_str());
    }
    invalid_flags.clear();

    if (not window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO,
            "Unable to create a window: %s",
            window.error().c_str());
    }

    // load the renderer from config settings
    auto renderer = make_renderer_from_config(
            config_path, *window,
            std::back_inserter(invalid_flags));
    for (auto const & name : invalid_flags) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
            "No renderer flag named \"%s\", skipping",
            name.c_str());
    }

    if (not renderer) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
            "Unable to create a renderer: %s",
            renderer.error().c_str());
    }

    SDL_Color const blue{ 66, 135, 245, 0 };
    SDL_SetRenderDrawColor(*renderer, blue.r, blue.g, blue.b, blue.a);
    SDL_RenderClear(*renderer);
    SDL_RenderPresent(*renderer);

    SDL_Delay(2000);

    SDL_DestroyRenderer(*renderer);
    SDL_DestroyWindow(*window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
