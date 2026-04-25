cmake_minimum_required(VERSION 3.20)
project(BDEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    main.cpp
    src/Core/Editor.cpp
    src/Core/Engine.cpp
    src/Core/Physics.cpp
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
)

# Libraries
set(RAYLIB_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/raylib/lib")
set(LUA_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Build/Lua/Debug")

target_link_libraries(BDEngine PRIVATE
    "${RAYLIB_LIB_DIR}/raylib.lib"
    "${LUA_LIB_DIR}/Lua.lib"
    winmm
)

# Compile definitions and flags
if(MSVC)
    target_compile_options(BDEngine PRIVATE /bigobj)
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

# Asset Copying
add_custom_command(TARGET BDEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:BDEngine>/assets"
    COMMENT "Copying assets to build directory..."
)

add_custom_command(TARGET BDEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/EditorConfig.ini"
        "$<TARGET_FILE_DIR:BDEngine>/EditorConfig.ini"
    COMMENT "Copying config to build directory..."
)
