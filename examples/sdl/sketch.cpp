// frameworkds
#include "raisin/raisin.hpp"
#include <SDL2/SDL.h>

// data types
#include <string>

// resource handles
#include <vector>

// algorithms
#include <iterator>

// load the subsystem flags from file
bool init_sdl(std::string const & config_path)
{
    using namespace raisin;
    std::vector<std::string> invalid_names;
    auto init_result = init_sdl_from_config(
            config_path,
            std::back_inserter(invalid_names));
    for (auto const & name : invalid_names) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
            "No subsystem named \"%s\", skipping",
            name.c_str());
    }
    if (init_result) {
        return true;
    }
    // some other error on SDL side occured when initializing
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
        "Unable to initialize SDL: %s",
        init_result.error().c_str());
    return false;
}

// load the window from config settings
SDL_Window * load_window(std::string const & config_path)
{
    using namespace raisin;
    std::vector<std::string> invalid_names;
    auto window_result = make_window_from_config(
            config_path,
            std::back_inserter(invalid_names));
    for (auto const & name : invalid_names) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
            "No window flag named \"%s\", skipping",
            name.c_str());
    }
    if (window_result) {
        return *window_result;
    }
    SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO,
        "Unable to create a window: %s",
        window_result.error().c_str());

    return nullptr;
}

// load the renderer from config settings
SDL_Renderer * load_renderer(SDL_Window * window,
                             std::string const & config_path)
{
    using namespace raisin;
    // load the renderer from config settings
    std::vector<std::string> invalid_names;
    auto renderer_result = make_renderer_from_config(
            config_path, window,
            std::back_inserter(invalid_names));
    for (auto const & name : invalid_names) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ASSERT,
            "No renderer flag named \"%s\", skipping",
            name.c_str());
    }
    if (renderer_result) {
        return *renderer_result;
    }
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
        "Unable to create a renderer: %s",
        renderer_result.error().c_str());
    return nullptr;
}

void cleanup(SDL_Window * window, SDL_Renderer * renderer)
{
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

void draw(SDL_Renderer * renderer)
{
    SDL_Color const blue{ 66, 135, 245, 0 };
    SDL_SetRenderDrawColor(renderer, blue.r, blue.g, blue.b, blue.a);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

int main()
{
    std::string const config_path = "../assets/config.toml";

    // load resources;
    if (not init_sdl(config_path)) {
        return EXIT_FAILURE;
    }
    SDL_Window * const window = load_window(config_path);
    if (not window) {
        cleanup(nullptr, nullptr);
        return EXIT_FAILURE;
    }
    SDL_Renderer * const renderer = load_renderer(window, config_path);
    if (not renderer) {
        cleanup(window, nullptr);
        return EXIT_FAILURE;
    }

    draw(renderer);
    SDL_Delay(2000);
    cleanup(window, renderer);

    return EXIT_SUCCESS;
}
