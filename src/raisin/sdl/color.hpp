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
#include <span>

inline namespace raisin {

inline constexpr std::size_t
size(SDL_Color const & color)
{
    return 4;
}

inline constexpr std::uint8_t *
data(SDL_Color & color)
{
    return &color.r;
}

inline constexpr std::uint8_t const *
data(SDL_Color const & color)
{
    return &color.r;
}

inline constexpr std::uint8_t *
begin(SDL_Color & color)
{
    return &color.r;
}

inline constexpr std::uint8_t const *
begin(SDL_Color const & color)
{
    return &color.r;
}

inline constexpr std::uint8_t *
end(SDL_Color & color)
{
    return (&color.a) + 1;
}

inline constexpr std::uint8_t const *
end(SDL_Color const & color)
{
    return (&color.a) + 1;
}

template<>
inline expected<SDL_Color, std::string>
load_value<SDL_Color>(toml::table const & table,
                      std::string const & variable_path)
{
    SDL_Color color;
    auto load_result = load_array(table, variable_path, color);
    if (not load_result) {
        return unexpected{ load_result.error() };
    }
    return color;
}
}
