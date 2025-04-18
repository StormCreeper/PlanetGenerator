# GLAD for modern OpenGL Extension
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_API "gl=4.5" CACHE STRING "" FORCE)
add_subdirectory(glad)
set_property(TARGET glad PROPERTY FOLDER "dep")

# GLFW for window creation and management
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
set_property(TARGET glfw PROPERTY FOLDER "dep")

# GLM for basic mathematical operators
add_subdirectory(glm)

# Fast noise
set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE)
add_subdirectory(FastNoise2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)

add_subdirectory(imgui)
