cmake_minimum_required(VERSION 3.9)

project(PlanetGen LANGUAGES CXX)

find_package(OpenMP REQUIRED)

add_subdirectory(dep)

file(GLOB_RECURSE PROJECT_SRC
     "Sources/*.h"
     "Sources/*.cpp"
)

add_executable (PlanetGen ${PROJECT_SRC})

set_target_properties(PlanetGen PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# Copy the shader files in the binary location.

include_directories(${CMAKE_CURRENT_SOURCE_DIR} dep/stb_image/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} Sources/)

target_link_libraries(PlanetGen LINK_PRIVATE glad)

target_link_libraries(PlanetGen LINK_PRIVATE glfw)

target_link_libraries(PlanetGen LINK_PRIVATE glm)

target_link_libraries(PlanetGen LINK_PRIVATE IMGUI)

target_link_libraries(PlanetGen PRIVATE OpenMP::OpenMP_CXX)

target_link_libraries(PlanetGen PRIVATE FastNoise)