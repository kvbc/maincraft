cmake_minimum_required(VERSION 3.8.2)

project(maincraft)

set(CMAKE_C_FLAGS "-O3 -s -DNDEBUG -Wall -Wextra")

include_directories(lib/glfw-3.3.7/include)
include_directories(lib/cglm-0.8.5/include)
include_directories(lib/glad-4.0-core/include)
include_directories(lib/stb_image-2.2.7/include)

link_directories(build/glfw/src)
link_directories(build/cglm)

add_executable(${PROJECT_NAME}
    lib/stb_image-2.2.7/src/stb_image.c
	lib/glad-4.0-core/src/glad.c
    src/file.c
	src/main.c
	src/program.c
	src/camera.c
	src/world.c
	src/tex.c
    src/math.c
    src/text.c
)

target_link_libraries(${PROJECT_NAME} glfw3)
target_link_libraries(${PROJECT_NAME} cglm)