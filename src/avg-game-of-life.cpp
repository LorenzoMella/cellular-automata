// Very simple implementation of continuous, color-valued 2D Cellular Automata using Raylib


#include "utils.hpp"
#include "Board.hpp"

#include <argparse.hpp>

#include <raylib.h>
#include <raymath.h>
#include <imgui.h>
#include <rlImGui.h>


// Default config
int WinWidth    = 1920;
int WinHeight   = 1080;
int BlockSide   = 8;
double InitRate = 0.5;

// Other useful globals
int NCols;
int NRows;

float InterpolationPersistence = 0.35;
float ContrastCorrection       = 0.22f;
float NoiseLevel               = 0.05f;

Color AliveColor      = BLUE;
Color DeadColor       = WHITE;
Color BackgroundColor = {150, 150, 150, 255};


// Coordinate deltas of neighboring cells

// clang-format off
utils::Vec2i RelativePairs2[]
    {{-2, -2}, {-1, -2}, {0, -2}, {1, -2}, {2, -2},
     {-2, -1}, {-1, -1}, {0, -1}, {1, -1}, {2, -1},
     {-2,  0}, {-1,  0},          {1,  0}, {2,  0},
     {-2,  1}, {-1,  1}, {0,  1}, {1,  1}, {2,  1},
     {-2,  2}, {-1,  2}, {0,  2}, {1,  2}, {2,  2}};
// clang-format on


utils::Vec2i identify_block(Vector2 position)
{
    return {(int)(position.x / BlockSide), (int)(position.y / BlockSide)};
}


Color random_color()
{
    return Color {(unsigned char)((double)rand() / RAND_MAX * 255.0),
                  (unsigned char)((double)rand() / RAND_MAX * 255.0),
                  (unsigned char)((double)rand() / RAND_MAX * 255.0),
                  255};
}


Color color_update(Board<Color> *board, int i, int j)
{
    int r = 0, g = 0, b = 0;
    for (auto d : RelativePairs)
    {
        r += board->get(utils::rem(i + d.y, NRows), utils::rem(j + d.x, NCols)).r;
        g += board->get(utils::rem(i + d.y, NRows), utils::rem(j + d.x, NCols)).g;
        b += board->get(utils::rem(i + d.y, NRows), utils::rem(j + d.x, NCols)).b;
    }
    return Color {.r = (unsigned char)(r / 8.0f),
                  .g = (unsigned char)(g / 8.0f),
                  .b = (unsigned char)(b / 8.0f),
                  .a = 255};
}


void parse_options(int argc, char *argv[])
{
    ArgumentParser ap;

    ap.add_arg("width", 'W', true);
    ap.add_arg("height", 'H', true);
    ap.add_arg("block-side", 's', true);
    ap.add_arg("fill-rate", 'f', true);
    ap.add_arg("survive-rule", 'S', true);
    ap.add_arg("generate-rule", 'B', true);

    ap.parse_args(argc, argv);

    if (ap.has_optional("width"))
    {
        WinWidth = std::stoi(ap.get_optional("width").value());
    }
    if (ap.has_optional("height"))
    {
        WinHeight = std::stoi(ap.get_optional("height").value());
    }
    if (ap.has_optional("block-side"))
    {
        BlockSide = std::stoi(ap.get_optional("block-side").value());
    }
    if (ap.has_optional("fill-rate"))
    {
        InitRate = std::stof(ap.get_optional("init-rate").value());
    }

    NCols = WinWidth / BlockSide;
    NRows = WinHeight / BlockSide;
}


int main(int argc, char *argv[])
{
    parse_options(argc, argv);

    InitWindow(WinWidth, WinHeight, "Game Of Life");
    SetTargetFPS(60);
    rlImGuiSetup(true);

    Board<Color> board1 {&random_color, NRows, NCols, BoundaryRule::THOROIDAL};
    Board<Color> board2 {board1};

    Board<Color> *front {&board2}, *back {&board1};

    bool do_update_board = false;

    utils::Timer timer {0.05};

    while (!WindowShouldClose())
    {
        // Toggle game of life evolution
        if (IsKeyPressed(KEY_SPACE))
        {
            do_update_board = !do_update_board;
        }

        // Toggle cells
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            utils::Vec2i block_coors = identify_block(GetMousePosition());
            Color &c                 = front->get(block_coors.y, block_coors.x);
            c                        = ColorIsEqual(c, AliveColor) ? DeadColor : AliveColor;
        }

        // Update

        if (do_update_board)
        {
            // Check tick
            if (timer.timeout())
            {
                timer.reset();

                // Parallel updates on back_board
                for (int i = 0; i < NRows; ++i)
                {
                    for (int j = 0; j < NCols; ++j)
                    {
                        Color interp = ColorLerp(front->get(i, j), color_update(front, i, j),
                                                 InterpolationPersistence);
                        interp       = ColorLerp(interp, random_color(), NoiseLevel);
                        interp       = ColorContrast(interp, ContrastCorrection);
                        back->set(i, j, interp);
                    }
                }

                // Swap buffers
                Board<Color> *buf = front;
                front             = back;
                back              = buf;
            }
            else
            {
                timer.cum_time += GetFrameTime();
            }
        }

        // Rendering

        BeginDrawing();

        ClearBackground(BackgroundColor);

        Vector2 block_size {(float)BlockSide, (float)BlockSide};
        front->draw([](auto x) { return x; }, block_size);

        DrawFPS(24, 24);

        rlImGuiBegin();
        {
            if (ImGui::Begin("Settings##avg-game-of-life"))
            {
                ImGui::SliderFloat("InterpolationPersistence", &InterpolationPersistence, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("ContrastCorrection", &ContrastCorrection, -1.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("NoiseLevel", &NoiseLevel, 0.0f, 1.0f, "%.2f");
                ImGui::Separator();
                ImGui::SliderFloat("Timeout Time", &timer.timeout_time, 0.017f, 1.0f, "%.3f");
            }
            ImGui::End();
        }
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
}
