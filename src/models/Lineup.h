#pragma once
#include <string>
#include <vector>

struct Lineup {
    std::string id;
    std::string map;
    std::string type;          // smoke | molotov | flash
    std::string title;
    std::string description;
    std::vector<std::string> tags;
    std::string stance;        // standing | crouching | jumping
    std::string throw_type;    // left-click | right-click | jump-throw
    std::string position_img;
    std::string crosshair_img;
    std::string video_url;
};
