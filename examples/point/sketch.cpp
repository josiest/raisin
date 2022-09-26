// data types
#include <string>
#include "point.hpp"

// resource handles
#include <tl/expected.hpp>
#include <optional>

// deserialization
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
#include <raisin/raisin.hpp>
#include <filesystem>

// i/o
#include <iostream>

namespace fs = std::filesystem;

raisin::expected<point, std::string>
load(toml::table const & table, std::string const & name)
{
    point p;
    auto result = raisin::subtable(table, name)
      .and_then(raisin::load("x", p.x))
      .and_then(raisin::load("y", p.y));

    if (not result) {
        return raisin::unexpected(result.error());
    }
    return p;
}

int main()
{
    std::string const asset_path = "../assets/point.toml";
    auto table_result = raisin::parse_file(asset_path);
    if (not table_result) {
        std::cerr << "Tried to parse config at " << asset_path
                  << "but failed:\n" << table_result.error() << "\n";
        return EXIT_FAILURE;
    }
    auto player_spawn = load(*table_result, "player-spawn");
    if (not player_spawn) {
        std::cerr << "Tried to load player spawn point from "
                  << asset_path << " but failed:\n"
                  << player_spawn.error() << "\n";
        return EXIT_FAILURE;
    }
    auto enemy_spawn = load(*table_result, "enemy-spawn");
    if (not enemy_spawn) {
        std::cerr << "Tried to load enemy spawn point from "
                  << asset_path << " but failed:\n"
                  << enemy_spawn.error() << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "player spawn point: " << *player_spawn << "\n"
              << "enemy spawn point: " << *enemy_spawn << "\n";
    return EXIT_SUCCESS;
}
