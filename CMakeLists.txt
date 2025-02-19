cmake_minimum_required(VERSION 3.20)
project(minesweeper LANGUAGES C)

add_subdirectory(deps/raylib)

set(CMAKE_C_STANDARD 17)

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

if(WIN32)
  add_executable(${PROJECT_NAME} WIN32 src/main.c)
else()
  add_executable(${PROJECT_NAME} src/main.c)
endif()
target_link_libraries(${PROJECT_NAME} raylib)
target_include_directories(${PROJECT_NAME} PRIVATE deps)