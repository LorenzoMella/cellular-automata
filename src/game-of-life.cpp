// Very simple implementation of binary 2D Cellular Automata using Raylib


#include "utils.hpp"
#include "Board.hpp"
#include <argparse.hpp>

#include <bitset>
#include <raylib.h>
#include <raymath.h>
#include <imgui.h>
#include <rlImGui.h>


// Default config
int WinWidth   = 1920;
int WinHeight  = 1080;
int BlockSide  = 8;
float FillRate = 0.5;

// Other useful globals
int NRows;
int NCols;

std::bitset<9> SurviveRule  {"000001100"};
std::bitset<9> GenerateRule {"000001000"};

Color AliveColor      = BLUE;
Color DeadColor       = WHITE;
Color BackgroundColor = {150, 150, 150, 255};


constexpr size_t N_COUNT_PATTERNS = 9;

char const *SurviveRuleLabels[9]  {"0##S", "1##S", "2##S", "3##S", "4##S", "5##S", "6##S", "7##S", "8##S"};
char const *GenerateRuleLabels[9] {"0##G", "1##G", "2##G", "3##G", "4##G", "5##G", "6##G", "7##G", "8##G"};


utils::Vec2i identify_block(Vector2 position)
{
    return {(int)(position.x / BlockSide), (int)(position.y / BlockSide)};
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
        FillRate = std::stof(ap.get_optional("fill-rate").value());
    }
    if (ap.has_optional("survive-rule"))
    {
        SurviveRule = std::bitset<N_COUNT_PATTERNS>{ap.get_optional("survive-rule").value()};
    }
    if (ap.has_optional("generate-rule"))
    {
        GenerateRule = std::bitset<N_COUNT_PATTERNS>{ap.get_optional("generate-rule").value()};
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

    Board<int> board1 {0, 1, FillRate, NRows, NCols, BoundaryRule::THOROIDAL};
    Board<int> board2 {board1};

    Board<int> *front {&board2}, *back {&board1};

    bool do_update_board = false;

    bool survive_rule_checkboxes[N_COUNT_PATTERNS];
    bool generate_rule_checkboxes[N_COUNT_PATTERNS];

    for (int i = 0; i < N_COUNT_PATTERNS; ++i)
    {
        survive_rule_checkboxes[i]  = SurviveRule[i];
        generate_rule_checkboxes[i] = GenerateRule[i];
    }

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
            int &val                 = front->get(block_coors.y, block_coors.x);
            val                      = !val;
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
                        int n = front->count_neighbors(i, j, 1,
                                                       [](int a, int b) { return a == b; });

                        if (front->get(i, j) == 0)
                        {
                            back->set(i, j, (int)GenerateRule[n]);
                        }
                        else if (front->get(i, j), 1)
                        {
                            back->set(i, j, (int)SurviveRule[n]);
                        }
                        else
                        {
                            // Should never happen
                            TraceLog(LOG_WARNING, "Neither Alive nor Dead?");
                            back->set(i, j, front->get(i, j));
                        }
                    }
                }

                // Swap buffers
                Board<int> *buf = front;
                front           = back;
                back            = buf;
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
        front->draw([=](int x) { return x == 0 ? DeadColor : AliveColor; }, block_size);
        DrawFPS(24, 24);

        rlImGuiBegin();
        {
            ImGui::ShowDemoWindow();

            if (ImGui::Begin("Settings##game-of-life"))
            {

                ImGui::Text("Survive when surrounded by...");
                for (int i = 0; i < N_COUNT_PATTERNS; ++i)
                {
                    if (ImGui::Checkbox(SurviveRuleLabels[i], &survive_rule_checkboxes[i]))
                    {
                        SurviveRule[i] = survive_rule_checkboxes[i];
                    }
                    if (i < 8)
                    {
                        ImGui::SameLine();
                    }
                }

                ImGui::Text("Revive when surrounded by...");
                for (int i = 0; i < N_COUNT_PATTERNS; ++i)
                {
                    if (ImGui::Checkbox(GenerateRuleLabels[i], &generate_rule_checkboxes[i]))
                    {
                        GenerateRule[i] = generate_rule_checkboxes[i];
                    }
                    if (i < 8)
                    {
                        ImGui::SameLine();
                    }
                }

                ImGui::Separator();

                ImGui::Text("Boundary rule");
                // FIXME They both stay alive for some reason
                if (ImGui::RadioButton("Thoroidal topology", (bool)front->m_boundary_rule))
                {
                    board1.m_boundary_rule = BoundaryRule::THOROIDAL;
                    board2.m_boundary_rule = BoundaryRule::THOROIDAL;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Permanently alive border", (bool)front->m_boundary_rule))
                {
                    board1.m_boundary_rule = BoundaryRule::ALWAYS_ALIVE;
                    board2.m_boundary_rule = BoundaryRule::ALWAYS_ALIVE;
                }

                ImGui::Separator();

                ImGui::SliderFloat("Random Rate", &FillRate, 0.0f, 1.0f, "%.2f");
                ImGui::SameLine();
                if (ImGui::Button("Fill at random"))
                {
                    front->random_fill(FillRate, 0, 1);
                }

                ImGui::Separator();

                ImGui::SliderFloat("Timeout Time", &timer.timeout_time, 0.017f, 1.0f, "%.3f");
                ImGui::Separator();

                float col3[4];
                utils::col2arr(AliveColor, col3);
                ImGui::ColorEdit3("Alive Cell Color", col3);
                utils::arr2col(col3, AliveColor);
                utils::col2arr(DeadColor, col3);
                ImGui::ColorEdit3("Dead Cell Color", col3);
                utils::arr2col(col3, DeadColor);
            }
            ImGui::End();
        }
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
}
