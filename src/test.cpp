#include "raisin/system.hpp"
#include "raisin/future.hpp"
#include <SDL2/SDL.h>

#include <string>
#include <vector>

#include <iostream>
#include <iterator>

int main()
{
    using namespace raisin::serialization;

    std::vector<std::string> const subsystems{
        "video", "events", "animation", "geometry"
    };

    // write bad subsystem names to a list
    // in case they were specified incorrectly
    std::vector<std::string> not_subsystems;
    not_subsystems.reserve(subsystems.size());

    auto flags = parse_subsystem_flags(
            subsystems,
            std::back_inserter(not_subsystems));

    // log when a name that was specified isn't a subsystem
    auto log_bad_subsystem = [](std::string const & name) {
        std::cout << "No subsystem named " << name << "\n";
    };
    if (not flags) {
        std::cout << "raisin error: Unable to parse flags:\n";
        ranges::for_each(not_subsystems, log_bad_subsystem);
        return EXIT_FAILURE;
    }

    // some other error on SDL side occured
    if (not SDL_Init(*flags)) {
        std::cout << "SDL Error: Unable to initialize SDL: "
                  << SDL_GetError() << "\n";
        return EXIT_FAILURE;
    }
    SDL_Log("Initialized SDL succesfully");
    SDL_Quit();

    return EXIT_SUCCESS;
}
