cmake_minimum_required(VERSION 3.20)
project(BDEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Lua target - built from source as C++ to ensure compatibility with sol2 exceptions
set(LUA_SOURCES
    External/lua/lapi.c
    External/lua/lauxlib.c
    External/lua/lbaselib.c
    External/lua/lcode.c
    External/lua/lcorolib.c
    External/lua/lctype.c
    External/lua/ldblib.c
    External/lua/ldebug.c
    External/lua/ldo.c
    External/lua/ldump.c
    External/lua/lfunc.c
    External/lua/lgc.c
    External/lua/linit.c
    External/lua/liolib.c
    External/lua/llex.c
    External/lua/lmathlib.c
    External/lua/lmem.c
    External/lua/loadlib.c
    External/lua/lobject.c
    External/lua/lopcodes.c
    External/lua/loslib.c
    External/lua/lparser.c
    External/lua/lstate.c
    External/lua/lstring.c
    External/lua/lstrlib.c
    External/lua/ltable.c
    External/lua/ltablib.c
    External/lua/ltm.c
    External/lua/lundump.c
    External/lua/lutf8lib.c
    External/lua/lvm.c
    External/lua/lzio.c
)

add_library(Lua STATIC ${LUA_SOURCES})
# Compile Lua C files as C++ to match MSVC project settings
set_source_files_properties(${LUA_SOURCES} PROPERTIES LANGUAGE CXX)
target_include_directories(Lua PUBLIC External/lua)

# Main Engine Target Source files
set(SOURCES
    main.cpp
    src/Core/Editor.cpp
    src/Core/Engine.cpp
    src/Core/Physics.cpp
    src/Managers/InputManager.cpp
    src/Managers/ProjectManager.cpp
    src/Managers/ResourceManager.cpp
    src/Managers/SceneManager.cpp
    src/Graphics/RenderPipeline.cpp
    src/Graphics/ShaderBuilder.cpp
    External/imgui/imgui/imgui.cpp
    External/imgui/imgui/imgui_demo.cpp
    External/imgui/imgui/imgui_draw.cpp
    External/imgui/imgui/imgui_tables.cpp
    External/imgui/imgui/imgui_widgets.cpp
    External/rlimgui/rlImGui.cpp
)

add_executable(BDEngine ${SOURCES})

# Include directories
target_include_directories(BDEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/raylib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/External
    ${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/External/rlimgui
    ${CMAKE_CURRENT_SOURCE_DIR}/External/lua
    ${CMAKE_CURRENT_SOURCE_DIR}/External/json
)

# Libraries
set(RAYLIB_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/raylib/lib")

target_link_libraries(BDEngine PRIVATE
    "${RAYLIB_LIB_DIR}/raylib.lib"
    Lua
    winmm
)

# Compile definitions and flags
if(MSVC)
    target_compile_options(BDEngine PRIVATE /bigobj)
    target_link_options(BDEngine PRIVATE /PDB:NONE)
endif()

target_compile_definitions(BDEngine PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    SOL_ALL_SAFETIES_ON=1
)

# Output directory strategy
set_target_properties(BDEngine PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/Release"
)

# Asset and Configuration Copying
add_custom_command(TARGET BDEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:BDEngine>/assets"
    COMMENT "Copying assets to build directory..."
)

add_custom_command(TARGET BDEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/engine-configs"
        "$<TARGET_FILE_DIR:BDEngine>/engine-configs"
    COMMENT "Copying engine-configs to build directory..."
)

add_custom_command(TARGET BDEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/EditorConfig.ini"
        "$<TARGET_FILE_DIR:BDEngine>/EditorConfig.ini"
    COMMENT "Copying config to build directory..."
)

