// frameworkds
#include <raisin/raisin.hpp>
#include <raisin/sdl.hpp>
#include <SDL2/SDL.h>

// data types
#include <string>

// resource handles
#include <vector>

// algorithms
#include <iterator>

// i/o
#include <iostream>

void log_bad_flags(std::string const & flag_type,
                   std::vector<std::string> const & invalid_names)
{
    for (auto const & name : invalid_names) {
        std::cerr << "No " << flag_type << " flag named "
                  << name << ", skipping\n";
    }
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
    SDL_Color constexpr blue{ 66, 135, 245, 255 };
    SDL_SetRenderDrawColor(renderer, blue.r, blue.g, blue.b, blue.a);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

int main()
{
    std::string const config_path = "../assets/config.toml";

    std::vector<std::string> invalid_subsystem_names;
    auto write_subsystems = std::back_inserter(invalid_subsystem_names);

    SDL_Window * window = nullptr;
    std::vector<std::string> invalid_window_names;
    auto write_window_flags = std::back_inserter(invalid_window_names);

    SDL_Renderer * renderer = nullptr;
    std::vector<std::string> invalid_renderer_names;
    auto write_renderer_flags = std::back_inserter(invalid_renderer_names);

    auto result = raisin::parse_file(config_path)
        .and_then(raisin::sdl::init_sdl("system", write_subsystems))
        .and_then(raisin::sdl::load_window("window", window, write_window_flags))
        .and_then(raisin::sdl::load_renderer("renderer", window, renderer, write_renderer_flags));

    log_bad_flags("subsystem", invalid_subsystem_names);
    log_bad_flags("window", invalid_window_names);
    log_bad_flags("renderer", invalid_renderer_names);

    if (not result) {
        std::cerr << "Couldn't load resources: " << result.error() << "\n";
        cleanup(window, renderer);
        return EXIT_FAILURE;
    }

    draw(renderer);
    SDL_Delay(2000);
    cleanup(window, renderer);

    return EXIT_SUCCESS;
}
