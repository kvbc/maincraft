cmake_minimum_required(VERSION 3.8.2)

project(maincraft)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_DEBUG_FLAGS   "-g -Wall -Wextra -pedantic")
set(CMAKE_C_RELEASE_FLAGS "-O3 -s -DNDEBUG")

include_directories(lib/glfw-3.3.7/include)
include_directories(lib/cglm-0.8.5/include)
include_directories(lib/glad-4.0-core/include)
include_directories(lib/stb_image-2.2.7/include)
include_directories(lib/FastNoiseLite/include)

link_directories(build/glfw/src)
link_directories(build/cglm)

add_executable(${PROJECT_NAME}
    lib/stb_image-2.2.7/src/stb_image.c
    lib/glad-4.0-core/src/glad.c
    lib/FastNoiseLite/src/FastNoiseLite.c
    src/file.c
    src/main.c
    src/program.c
    src/camera.c
    src/world.c
    src/tex.c
    src/text.c
    src/block.c
)

target_link_libraries(${PROJECT_NAME} glfw3)
target_link_libraries(${PROJECT_NAME} cglm)