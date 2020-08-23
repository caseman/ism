cmake_minimum_required(VERSION 3.10)

project(Ism VERSION 0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include_directories(
    src
)

add_library(fastrng STATIC src/cccompat.h src/fastrng.h src/fastrng.c)
add_library(perlin_noise STATIC src/perlin.h src/perlin.c)

# file(GLOB perlin_noise_sources "lib/perlin-noise/src/*.[hc]")

configure_file(src/ism.h.in include/ism.h)
add_executable(ism src/ism.c)

target_link_libraries(ism fastrng perlin_noise)
target_include_directories(ism PUBLIC
    "${PROJECT_BINARY_DIR}/include"
)

