// Very simple implementation of Conway's Game of Life using Raylib

// TODO Comparison should be by symbol, not by Color structure, which is only
// useful for drawing, by this point.
// In the future, Colors can be introduced in the logic itself, by generalizing
// update rules. The HSL format is probably better suited for this purpose.


#include <algorithm>
#include <getopt.h>
#include <vector>

#include <raylib.h>
#include <raymath.h>

#include <imgui.h>
#include <rlImGui.h>

#include "argparse.hpp"

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


struct Vec2i
{
    int x;
    int y;
};


inline int rem(int a, int b) // expects b > 0
{
    return (a + b) % b;
}


inline void arr2col(float col3[], Color &c)
{
    c.r = (unsigned char)(col3[0] * 255.0f);
    c.g = (unsigned char)(col3[1] * 255.0f);
    c.b = (unsigned char)(col3[2] * 255.0f);
    c.a = 255;
}


inline void col2arr(Color &c, float col3[])
{
    col3[0] = (float)c.r / 255.0f;
    col3[1] = (float)c.g / 255.0f;
    col3[2] = (float)c.b / 255.0f;
}


struct Cell
{
    float intensity;
    float stimulation;
};


template <typename T>
class Board
{
  public:
    Board() { };

    Board(Board const &c) = default;

    Board(T const &init_val, int n_rows, int n_cols)
      : m_arr(n_rows * n_cols),
        m_n_rows {n_rows},
        m_n_cols {n_cols}
    {
        std::fill(m_arr.begin(), m_arr.end(), init_val);
    }

    Board(T const &init_val, T const &alt_val, int n_rows, int n_cols, double rate)
      : m_arr(n_rows * n_cols),
        m_n_rows {n_rows},
        m_n_cols {n_cols}
    {
        for (size_t i = 0; i < m_arr.size(); ++i)
        {
            m_arr[i] = (rand() / (float)RAND_MAX) <= rate ? alt_val : init_val;
        }
    }

    Board(T const &init_val, int n_rows, int n_cols, T (*random_value)())
      : m_arr(n_rows * n_cols),
        m_n_rows {n_rows},
        m_n_cols {n_cols}
    {
        reset_at_random(random_value);
    }

    ~Board() = default;

    T &get(int i, int j) { return m_arr[i * m_n_cols + j]; }
    T const &get(int i, int j) const { return m_arr[i * m_n_cols + j]; }
    T &set(int i, int j, T const &val) { return (m_arr[i * m_n_cols + j] = val); }

    void reset_at_random(T (*random_value)())
    {
        for (size_t i = 0; i < m_arr.size(); ++i)
        {
            m_arr[i] = random_value();
        }
    }

    void reset_with_rectangle(T const &init_val, T const &rect_val, Vec2i top_left, Vec2i bottom_right);

    int count_neighbors(int i, int j, T const &to, bool (*comp)(T a, T b));

  private:
    std::vector<T> m_arr;

    int m_n_rows;
    int m_n_cols;
};


template <typename T>
void Board<T>::reset_with_rectangle(T const &init_val, T const &rect_val, Vec2i top_left, Vec2i bottom_right)
{
    std::fill(m_arr.begin(), m_arr.end(), init_val);
    for (int i = top_left.y; i < bottom_right.y; ++i)
    {
        for (int j = top_left.x; j < (bottom_right.y - i < 5 ? bottom_right.x + 10 : bottom_right.x); ++j)
        {
            get(i, j) = rect_val;
        }
    }
}


template <typename T>
int Board<T>::count_neighbors(int i, int j, T const &to, bool (*comp)(T a, T b))
{
    int n = -(int)comp(get(rem(i, NRows), rem(j, NCols)), to);
    for (int h = -NeighL1Radius; h <= NeighL1Radius; ++h)
    {
        for (int k = -NeighL1Radius; k <= NeighL1Radius; ++k)
        {
            n += (int)comp(get(rem(i + h, NRows), rem(j + k, NCols)), to);
        }
    }
    return n;
}


Cell generate_random_cell()
{
    return Cell {
        .intensity   = (float)rand() / RAND_MAX,
        .stimulation = (float)rand() / RAND_MAX * ExcitationTime,
    };
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
            int neighbor_i = rem(i + h, NRows);
            int neighbor_j = rem(j + k, NCols);
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


void board_draw(Board<Cell> &b)
{
    Vector2 size {(float)BlockSide - 2, (float)BlockSide - 2};
    Vector2 position {1, 1};
    for (int i = 0; i < NRows; ++i)
    {
        for (int j = 0; j < NCols; ++j)
        {
            Color c = ColorFromHSV(360.0f * b.get(i, j).stimulation / (ExcitationTime + RefractoryTime),
                                   1.0f,
                                   b.get(i, j).intensity);
            DrawRectangleV(position, size, c);
            position.x += BlockSide;
        }
        position.x = 1;
        position.y += BlockSide;
    }
}


struct Timer
{
    float cum_time {0.0f};
    float timeout_time;

    Timer(float timeout_time) : timeout_time {timeout_time} { }

    bool timeout() const { return cum_time >= timeout_time; }
    void reset() { cum_time = 0.0f; }
};


Vec2i identify_block(Vector2 position)
{
    return {int(position.x / BlockSide), int(position.y / BlockSide)};
}


struct option long_options[] {{"width", required_argument, nullptr, 'W'},
                              {"height", required_argument, nullptr, 'H'},
                              {"block-size", required_argument, nullptr, 's'},
                              {"init-rate", required_argument, nullptr, 'r'},
                              {0, 0, 0, 0}};


int main(int argc, char *argv[])
{
    ArgumentParser ap;

    // It does not parse correctly. I believe that the error is in the fact that
    // the strings here are r-values. They are probably only temporarily copied.
    ap.add_arg("width", 'W', true);
    ap.add_arg("height", 'H', true);
    ap.add_arg("block-side", 's', true);
    ap.add_arg("init-rate", 'r', true);

    ap.parse_args(argc, argv);

    if (ap.has_optional("width"))
    {
        WinWidth = std::strtol(ap.get_optional("width").value().c_str(), nullptr, 10);
    }
    if (ap.has_optional("height"))
    {
        WinWidth = std::strtol(ap.get_optional("height").value().c_str(), nullptr, 10);
    }
    if (ap.has_optional("block-side"))
    {
        WinWidth = std::strtol(ap.get_optional("block-side").value().c_str(), nullptr, 10);
    }
    if (ap.has_optional("init-rate"))
    {
        WinWidth = std::strtod(ap.get_optional("init-rate").value().c_str(), nullptr);
    }

    NCols = WinWidth / BlockSide;
    NRows = WinHeight / BlockSide;

    InitWindow(WinWidth, WinHeight, "Game Of Life");
    SetTargetFPS(60);
    rlImGuiSetup(true);

    Board<Cell> board1 {{0.0f, 0.0f}, NRows, NCols, generate_random_cell};

    Board<Cell> board2 {board1};

    Board<Cell> *front {&board2}, *back {&board1};

    bool do_update_board = false;

    Timer timer {0.5f};

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
            Vec2i block_coors = identify_block(GetMousePosition());
            Cell &cell        = front->get(block_coors.y, block_coors.x);
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

            // Check tick
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

        board_draw(*front);
        DrawFPS(24, 24);

        // ImGui here
        rlImGuiBegin();

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
                front->reset_at_random(generate_random_cell);
            }
            ImGui::SameLine();
            if (ImGui::Button("Fill with rectangle"))
            {
                front->reset_with_rectangle({0.0f, 0.0f},
                                            {10.0f, 0.0f},
                                            {NCols / 2 - 5, NRows / 2 - 20},
                                            {NCols / 2 + 5, NRows / 2 + 20});
            }
        }

        ImGui::End();

        rlImGuiEnd();
        // ImGui ends here

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
}
