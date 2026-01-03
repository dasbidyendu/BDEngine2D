#pragma once

// 1. Prevent Windows from defining most of its junk
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX

#include <windows.h>

// 2. Explicitly kill the names that Raylib/Raygui use
#undef CloseWindow
#undef ShowCursor
#undef Rectangle
#undef Transparent
#undef PlaySound