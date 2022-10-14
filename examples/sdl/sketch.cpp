// frameworks
#include <SDL2/SDL.h>
#include <raisin/raisin.hpp>
#include <raisin/sdl.hpp>

// data types
#include <string>

// type constraints
#include <ranges>

// data structures and algorithms
#include <array>
#include <iterator>

// i/o
#include <iostream>

namespace ranges = std::ranges;

template<ranges::input_range range>
requires std::same_as<ranges::range_value_t<range>, std::string>
void log_bad_flags(std::string const & flag_type, range && invalid_names)
{
    for (std::string const & name : invalid_names) {
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

void draw(SDL_Renderer * renderer, SDL_Color const & color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

int main()
{
    std::string const config_path = "../assets/config.toml";

    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;

    std::vector<std::string> invalid_subsystem_flagnames;
    std::vector<std::string> invalid_window_flagnames;
    std::vector<std::string> invalid_renderer_flagnames;

    SDL_Color draw_color;
    auto result = raisin::parse_file(config_path)
        .and_then(raisin::sdl::init_sdl("system",
            std::back_inserter(invalid_subsystem_flagnames)))
        .and_then(raisin::sdl::load_window("window", window,
            std::back_inserter(invalid_window_flagnames)))
        .and_then(raisin::sdl::load_renderer("renderer", window, renderer,
            std::back_inserter(invalid_renderer_flagnames)))
        .and_then(raisin::load(
            "draw.color", draw_color));

    log_bad_flags("subsystem", invalid_subsystem_flagnames);
    log_bad_flags("window", invalid_window_flagnames);
    log_bad_flags("renderer", invalid_renderer_flagnames);

    if (not result) {
        std::cerr << "Couldn't load resources: " << result.error() << "\n";
        cleanup(window, renderer);
        return EXIT_FAILURE;
    }

    draw(renderer, draw_color);
    SDL_Delay(2000);
    cleanup(window, renderer);

    return EXIT_SUCCESS;
}
