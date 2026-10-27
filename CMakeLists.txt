set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
cmake_minimum_required(VERSION 3.16...3.30)

project(Kucyki LANGUAGES CXX)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(SFML)

add_executable(gra
    src/main.cpp
    include/game_config.hpp
    include/json_parser.hpp
    include/questions.hpp
    include/game_state.hpp
    include/render_helpers.hpp
    include/menu_screens.hpp
    include/player_selection.hpp
    include/gameplay.hpp
    include/game_rendering.hpp
    include/game_over.hpp
)

target_include_directories(gra PRIVATE include)
target_compile_features(gra PRIVATE cxx_std_17)
target_link_libraries(gra PRIVATE SFML::Graphics SFML::Audio)

if(MSVC)
    target_compile_options(gra PRIVATE /utf-8)
endif()

add_custom_command(TARGET gra POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:gra>/resources"
    VERBATIM)