cmake_minimum_required(VERSION "3.20")

project(2d-cellular-automata)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_compile_options(-O3 -Wall -pedantic)
else()
  add_compile_options(-O0 -g -Wall -pedantic)
endif()

### Third party dependencies

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

## Raylib
set(CUSTOMIZE_BUILD          ON CACHE INTERNAL "")
set(BUILD_EXAMPLES          OFF CACHE INTERNAL "")
set(BUILD_GAMES             OFF CACHE INTERNAL "")
set(USE_EXTERNAL_GLFW       OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS       OFF CACHE INTERNAL "")
set(BUILD_STATIC_LIBS        ON CACHE INTERNAL "")
set(SUPPORT_GESTURES_SYSTEM OFF CACHE INTERNAL "")
if(CMAKE_BUILD_TYPE STREQUAL Release)
  set(SUPPORT_TRACELOG ON)
endif()

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG "5.5"
)

FetchContent_MakeAvailable(raylib)

## Dear ImGui

FetchContent_Declare(
  dearimgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG "v1.92.7"
)

FetchContent_MakeAvailable(dearimgui)

## rlImGui

FetchContent_Declare(
  rlimgui
  GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
  GIT_TAG "Raylib_5_5"
)

FetchContent_MakeAvailable(rlimgui)

# Build the whole thing into a monolithic library

file(GLOB dearimgui-headers "${dearimgui_SOURCE_DIR}/*.h")
file(GLOB dearimgui-sources "${dearimgui_SOURCE_DIR}/*.cpp")

add_library(dearimgui STATIC ${dearimgui-headers} ${dearimgui-sources})
target_link_libraries(dearimgui PUBLIC raylib)

file(GLOB rlimgui-headers "${rlimgui_SOURCE_DIR}/*.h")
file(GLOB rlimgui-extras "${rlimgui_SOURCE_DIR}/extras/*.h")
file(GLOB rlimgui-sources "${rlimgui_SOURCE_DIR}/*.cpp")

add_library(rlimgui STATIC ${rlimgui-headers} ${rlimgui-extras} ${rlimgui-sources})
target_include_directories(rlimgui PRIVATE "${dearimgui_SOURCE_DIR}")
target_link_libraries(rlimgui PUBLIC raylib dearimgui)

### The programs

add_subdirectory("${CMAKE_SOURCE_DIR}/argparse++")

add_executable(game-of-life src/game-of-life.cpp)
target_link_libraries(game-of-life PRIVATE raylib argparse dearimgui rlimgui)
target_link_libraries(game-of-life PRIVATE "-lm" "-lGL" "-lpthread" "-ldl" "-lrt" "-lX11")
target_include_directories(game-of-life PRIVATE
  "argparse++" "${raylib_SOURCE_DIR}/include" "${dearimgui_SOURCE_DIR}" "${rlimgui_SOURCE_DIR}")

add_executable(avg-game-of-life src/avg-game-of-life.cpp)
target_link_libraries(avg-game-of-life PRIVATE raylib argparse dearimgui rlimgui)
target_link_libraries(avg-game-of-life PRIVATE "-lm" "-lGL" "-lpthread" "-ldl" "-lrt" "-lX11")
target_include_directories(avg-game-of-life PRIVATE
  "argparse++" "${raylib_SOURCE_DIR}/include" "${dearimgui_SOURCE_DIR}" "${rlimgui_SOURCE_DIR}")

add_executable(wiener-rosenblueth src/wiener-rosenblueth.cpp)
target_link_libraries(wiener-rosenblueth PRIVATE raylib argparse dearimgui rlimgui)
target_link_libraries(wiener-rosenblueth PRIVATE "-lm" "-lGL" "-lpthread" "-ldl" "-lrt" "-lX11")
target_include_directories(wiener-rosenblueth PRIVATE
  "argparse++" "${raylib_SOURCE_DIR}/include" "${dearimgui_SOURCE_DIR}" "${rlimgui_SOURCE_DIR}")
