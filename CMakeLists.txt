cmake_minimum_required(VERSION 3.30)
project(minesweeper LANGUAGES C)

add_subdirectory(deps/raylib)

set(CMAKE_C_STANDARD 17)

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} raylib)