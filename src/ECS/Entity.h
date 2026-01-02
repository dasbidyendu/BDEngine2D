#pragma once
#include <cstdint>


const int MAX_ENTITIES = 2000;

using Entity = unsigned int;

enum ComponentType : std::uint8_t {
    COMP_TRANSFORM = 0,
    COMP_SPRITE,
    COMP_VELOCITY,
    COMP_INPUT,
    COMP_UI,
    COMP_COUNT 
};