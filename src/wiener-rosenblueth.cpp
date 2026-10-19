// Very simple implementation of the discretized Wiener-Rosenblueth algorithm as a 2D Cellular Automaton


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

// Simulation globals
float IntensityDecay      = 0.5;
float ExcitationThreshold = 3.0;
float ExcitationTime      = 3.0;
float RefractoryTime      = 5.0;
int NeighL1Radius         = 1;

// Other useful globals
int NRows;
int NCols;

Color AliveColor      = BLUE;
Color DeadColor       = WHITE;
Color BackgroundColor = {150, 150, 150, 255};


struct Cell
{
    float intensity;
    float stimulation;
};


Cell generate_random_cell()
{
    return Cell {
        .intensity   = (float)rand() / RAND_MAX,
        .stimulation = (float)rand() / RAND_MAX * ExcitationTime,
    };
}


void reset_with_rectangle(Board<Cell> &board, Cell const &init_val, Cell const &rect_val,
                          utils::Vec2i top_left, utils::Vec2i bottom_right)
{
    board.fill(init_val);
    for (int i = top_left.y; i < bottom_right.y; ++i)
    {
        for (int j = top_left.x; j < (bottom_right.y - i < 5 ? bottom_right.x + 10 : bottom_right.x); ++j)
        {
            board.get(i, j) = rect_val;
        }
    }
}


static inline bool at_rest(Board<Cell> &board, int i, int j)
{
    return 0 == board.get(i, j).stimulation;
}


static inline bool is_excited(Board<Cell> &board, int i, int j)
{
    return 0 < board.get(i, j).stimulation && board.get(i, j).stimulation <= ExcitationTime;
}


static inline bool is_refractory(Board<Cell> &board, int i, int j)
{
    return ExcitationTime < board.get(i, j).stimulation && board.get(i, j).stimulation <= RefractoryTime;
}


Cell generate_updated_cell(Board<Cell> &board, int i, int j)
{
    Cell const &center_cell = board.get(i, j);
    Cell new_cell;

    new_cell.intensity = IntensityDecay * center_cell.intensity - (float)is_excited(board, i, j);
    for (int h = -NeighL1Radius; h <= NeighL1Radius; ++h)
    {
        for (int k = -NeighL1Radius; k <= NeighL1Radius; ++k)
        {
            int neighbor_i = utils::rem(i + h, NRows);
            int neighbor_j = utils::rem(j + k, NCols);
            new_cell.intensity += (float)is_excited(board, neighbor_i, neighbor_j) / NeighL1Radius;
        }
    }

    if (center_cell.stimulation == 0.0f)
    {
        new_cell.stimulation = (float)(new_cell.intensity >= ExcitationThreshold);
    }
    else if (center_cell.stimulation >= ExcitationTime + RefractoryTime)
    {
        new_cell.stimulation = 0.0f;
    }
    else
    {
        new_cell.stimulation += 1.0f;
    }

    return new_cell;
}


utils::Vec2i identify_block(Vector2 position)
{
    return {int(position.x / BlockSide), int(position.y / BlockSide)};
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


static Color cell_color_map(Cell const &cell)
{
    return ColorFromHSV(360.0f * cell.stimulation / (ExcitationTime + RefractoryTime),
                        1.0f,
                        cell.intensity);
}


int main(int argc, char *argv[])
{
    parse_options(argc, argv);

    InitWindow(WinWidth, WinHeight, "Game Of Life");
    SetTargetFPS(60);
    rlImGuiSetup(true);

    Cell const dead_cell = {0.0f, 0.0f};

    Board<Cell> board1 {dead_cell, NRows, NCols, BoundaryRule::THOROIDAL};

    Board<Cell> board2 {board1};

    Board<Cell> *front {&board2}, *back {&board1};

    bool do_update_board = false;

    utils::Timer timer {0.5f};

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
            Cell &cell               = front->get(block_coors.y, block_coors.x);
            if (cell.intensity == 0.0f)
            {
                cell.intensity = 0.0f;
            }
            else
            {
                cell.intensity = 1.0f;
            }
        }

        // Update phase
        if (do_update_board)
        {
            if (timer.timeout())
            {
                timer.reset();

                // Parallel updates on back_board
                for (int i = 0; i < NRows; ++i)
                {
                    for (int j = 0; j < NCols; ++j)
                    {
                        Cell new_cell = generate_updated_cell(*front, i, j);
                        back->set(i, j, new_cell);
                    }
                }

                // Swap buffers
                Board<Cell> *buf = front;
                front            = back;
                back             = buf;
            }
            else
            {
                timer.cum_time += GetFrameTime();
            }
        }

        // Graphics rendering below

        BeginDrawing();

        ClearBackground(BackgroundColor);

        Vector2 block_size {(float)BlockSide, (float)BlockSide};
        front->draw(cell_color_map, block_size);

        DrawFPS(24, 24);

        rlImGuiBegin();
        {
            if (ImGui::Begin("Settings##avg-game-of-life"))
            {
                ImGui::SeparatorText("Simulation constants");
                ImGui::SliderFloat("Intensity Decay", &IntensityDecay, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Excitation Threshold", &ExcitationThreshold, 0.0f, 10.0f, "%.2f");
                ImGui::SliderInt("Neighborhood L1 radius", &NeighL1Radius, 0, 10);
                ImGui::SeparatorText("Simulation times");
                ImGui::SliderFloat("Excitation Time", &ExcitationTime, 0.0f, 10.0f, "%.2f");
                ImGui::SliderFloat("Refractory Time", &RefractoryTime, 0.0f, 10.0f, "%.2f");
                ImGui::SliderFloat("Timeout Time", &timer.timeout_time, 0.017f, 1.0f, "%.3f");
                ImGui::Separator();
                if (ImGui::Button("Fill at random"))
                {
                    front->random_fill(generate_random_cell);
                }
                ImGui::SameLine();
                if (ImGui::Button("Fill with rectangle"))
                {
                    reset_with_rectangle(*front,
                                         {0.0f, 0.0f},
                                         {10.0f, 0.0f},
                                         {NCols / 2 - 5, NRows / 2 - 20},
                                         {NCols / 2 + 5, NRows / 2 + 20});
                }
            }
            ImGui::End();
        }
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
}
