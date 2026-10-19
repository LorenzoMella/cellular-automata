# Small 2D Cellular Automata

Three small applications that allow experimenting with 2D cellular automata rules similar to (and including) [Conway's Game Of Life](https://en.wikipedia.org/wiki/Conway's_Game_of_Life).

The rationale for making this was that I set for myself a personal challenge to build a program with a [Dear ImGui](https://github.com/ocornut/imgui/) interface in a few-hour timeframe. This was easily accomplished using the sleek [Raylib](https://www.raylib.com/) library and its unofficial Raylib backend for ImGui ([rlImGui](https://github.com/raylib-extras/rlImGui/tree/main/]).

## Build (Linux)

Build with [CMake](https://cmake.org/):

```sh
cd cellular-automata  # i.e., the directory where this file is located
cmake -B build
cmake --build build
```

You will subsequently find the standalone executables in the newly created `build` directory.

## Build (Windows)

**Coming soon**.

## How it works

The *Space bar* pauses and unpauses the simulation. Use the GUI options and sliders to change the *rules* (where applicable) and the other simulation parameters.

## Interesting rules

Some rules to try out with `game-of-life`.

| Rule Name                                | Survives  | Born    | Survives (binary) | Born (binary) |
|------------------------------------------|-----------|---------|-------------------|---------------|
| Conway's Rule (the original)             | 2, 3      | 3       | `000001100`       | `000001000`   |
| Conway with improved persistence         | 2, 3, 7   | 3, 6    | `010001100`       | `001001000`   |
| Conway with high-crowd survival          | 2,3,6,7,8 | 3       | `111001100`       | `000001000`   |
| Slowly-expanding cohesive blobs          | 3,5       | 3,4     | `000100100`       | `000011000`   |
| Very slowly-expanding cohesive blobs     | 2,5       | 3,4     | `000101000`       | `000011000`   |
| Day-and-Night (overworld map generation) | 3,4,6,7,8 | 3,6,7,8 | `111011000`       | `111001000`   |
