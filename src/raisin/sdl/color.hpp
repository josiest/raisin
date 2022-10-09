#pragma once

// library
#include "raisin/future.hpp"
#include "raisin/fundamental_types.hpp"

// data types
#include <SDL2/SDL_pixels.h>
#include <string>
#include <cstdint>

// deserialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>

namespace raisin {

template<>
expected<SDL_Color, std::string>
load_value<SDL_Color>(toml::table const & table,
                      std::string const & variable_path)
{
    SDL_Color color;
    std::uint8_t * const begin_color = &color.r;

    auto color_result = load_array<std::uint8_t>(
        table, variable_path, begin_color);
    if (not color_result) {
        return unexpected{ color_result.error() };
    }

    return color;
}
}
