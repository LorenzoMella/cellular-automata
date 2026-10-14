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
int WinWidth          = 1920;
int WinHeight         = 1080;
int BlockSide         = 8;
double InitRate       = 0.5;

// Other useful globals
int NRows;
int NCols;

float InterpolationPersistence = 0.1f;
float ContrastCorrection       = 0.0f;
float NoiseLevel               = 0.2f;

Color AliveColor      = BLUE;
Color DeadColor       = WHITE;
Color BackgroundColor = {150, 150, 150, 255};


inline bool float_slider(char const *label, float *val, float min, float max, char const *fmt)
{
    return ImGui::SliderScalar(label, ImGuiDataType_Float, val, &min, &max, fmt);
}


struct Vec2i
{
    int x;
    int y;
};


Vec2i RelativePairs[] {{-1, -1}, { 0, -1}, { 1, -1},
                       {-1,  0},           { 1,  0},
                       {-1,  1}, { 0,  1}, { 1,  1}};


Vec2i RelativePairs2[]
    {{-2, -2}, {-1, -2}, {0, -2}, {1, -2}, {2, -2},
     {-2, -1}, {-1, -1}, {0, -1}, {1, -1}, {2, -1},
     {-2,  0}, {-1,  0},          {1,  0}, {2,  0},
     {-2,  1}, {-1,  1}, {0,  1}, {1,  1}, {2,  1},
     {-2,  2}, {-1,  2}, {0,  2}, {1,  2}, {2,  2}};


inline int rem(int a, int b)    // expects b > 0
{
    return (a + b) % b;
}


template <typename T>
class Board
{
public:

    Board() {};

    Board(Board const& c) = default;

    Board(T const &init_val, int n_rows, int n_cols)
      : m_n_rows{n_rows},
        m_n_cols{n_cols},
        m_arr(n_rows * n_cols)
    {
        m_arr.fill(init_val);
    }

    Board(T const &init_val, T const &alt_val, int n_rows, int n_cols, double rate)
      : m_n_rows{n_rows},
        m_n_cols{n_cols},
        m_arr(n_rows * n_cols)
    {
        for (size_t i = 0; i < m_arr.size(); ++i) {
            m_arr[i] = (rand() / (float)RAND_MAX) <= rate ? alt_val : init_val;
        }
    }

    Board(T const &init_val, int n_rows, int n_cols, T (*random_value)())
      : m_n_rows {n_rows},
        m_n_cols {n_cols},
        m_arr(n_rows * n_cols)
    {
        for (size_t i = 0; i < m_arr.size(); ++i) {
            m_arr[i] = random_value();
        }
    }

    ~Board() = default;

    T&        get(int i, int j)               { return m_arr[i * m_n_cols + j]; }
    T const & get(int i, int j) const         { return m_arr[i * m_n_cols + j]; }
    T&        set(int i, int j, T const &val) { return (m_arr[i * m_n_cols + j] = val); }

    int count_neighbors(int i, int j, T const &to, bool (*comp)(T a, T b));

private:

    std::vector<T> m_arr;

    int m_n_rows;
    int m_n_cols;
};


template <typename T>
int Board<T>::count_neighbors(int i, int j, T const &to, bool (*comp)(T a, T b))
{
    int n = 0;
    for (auto d : RelativePairs) {
        n += (int)comp(get(rem(i + d.y, NRows), rem(j + d.x, NCols)), to);
    }
    return n;
}


void board_draw(Board<Color> &b)
{
    Vector2 size {(float)BlockSide - 2, (float)BlockSide - 2};
    Vector2 position {1, 1};
    for (int i = 0; i < NRows; ++i) {
        for (int j = 0; j < NCols; ++j) {
            DrawRectangleV(position, size, b.get(i, j));
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

    Timer(float timeout_time)
    : timeout_time {timeout_time} {}

    bool timeout() const { return cum_time >= timeout_time; }
    // TODO Should it preserve the remainder?
    void reset() { cum_time = 0.0f; }
};


Vec2i identify_block(Vector2 position)
{
    return {int(position.x / BlockSide), int(position.y / BlockSide)};
}


struct option long_options[] {
    {"width",         required_argument, nullptr, 'W'},
    {"height",        required_argument, nullptr, 'H'},
    {"block-size",    required_argument, nullptr, 's'},
    {"init-rate",     required_argument, nullptr, 'r'},
    {0, 0, 0, 0}
};


Color color_update(Board<Color> *board, int i, int j)
{
    int r = 0, g = 0, b = 0;
    for (auto d : RelativePairs) {
        r += board->get(rem(i + d.y, NRows), rem(j + d.x, NCols)).r;
        g += board->get(rem(i + d.y, NRows), rem(j + d.x, NCols)).r;
        b += board->get(rem(i + d.y, NRows), rem(j + d.x, NCols)).r;
    }
    return Color{(unsigned char)(r / 8.0f),
                 (unsigned char)(g / 8.0f),
                 (unsigned char)(b / 8.0f),
                 255};
}


Color random_color()
{
    return Color {(unsigned char)((double)rand() / RAND_MAX * 255.0),
                  (unsigned char)((double)rand() / RAND_MAX * 255.0),
                  (unsigned char)((double)rand() / RAND_MAX * 255.0),
                  255};
}


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
        WinWidth = std::strtol(ap.get_optional("width").value().c_str(),
                               nullptr,
                               10);
    }
    if (ap.has_optional("height"))
    {
        WinWidth = std::strtol(ap.get_optional("height").value().c_str(),
                               nullptr,
                               10);
    }
    if (ap.has_optional("block-side"))
    {
        WinWidth = std::strtol(ap.get_optional("block-side").value().c_str(),
                               nullptr,
                               10);
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

    Board<Color> board1 {DeadColor, NRows, NCols, &random_color};
    Board<Color> board2 {board1};

    Board<Color> *front {&board2}, *back {&board1};

    bool do_update_board = false;

    Timer timer {0.05};

    while (!WindowShouldClose()) {

        // Toggle game of life evolution
        if (IsKeyPressed(KEY_SPACE)) {
            do_update_board = !do_update_board;
        }

        // Toggle cells
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vec2i block_coors = identify_block(GetMousePosition());
            Color &c = front->get(block_coors.y, block_coors.x);
            if (ColorIsEqual(c, AliveColor)) {
                c = DeadColor;
            } else {
                c = AliveColor;
            }
        }

        // Update phase
        if (do_update_board) {

            // Check tick
            if (timer.timeout()) {

                timer.reset();

                // Parallel updates on back_board
                for (int i = 0; i < NRows; ++i) {
                    for (int j = 0; j < NCols; ++j) {
                        Color interp = ColorLerp(front->get(i, j),
                                                 color_update(front, i, j),
                                                 InterpolationPersistence);
                        interp = ColorLerp(interp,
                                           random_color(),
                                           NoiseLevel);
                        interp = ColorContrast(interp, ContrastCorrection);
                        back->set(i, j, interp);
                    }
                }

                // Swap buffers
                Board<Color> *buf = front;
                front = back;
                back = buf;

            } else {
                timer.cum_time += GetFrameTime();
            }
        }

        // Graphics rendering below

        ///////////////
        BeginDrawing();
        ///////////////

        ClearBackground(BackgroundColor);

        board_draw(*front);
        DrawFPS(24, 24);

        // ImGui here

        rlImGuiBegin();

        if (ImGui::Begin("Settings##avg-game-of-life")) {
            float_slider("InterpolationPersistence", &InterpolationPersistence, 0.0f, 1.0f, "%.2f");
            float_slider("ContrastCorrection", &ContrastCorrection, -1.0f, 1.0f, "%.2f");
            float_slider("NoiseLevel", &NoiseLevel, 0.0f, 1.0f, "%.2f");
            ImGui::Separator();
            float_slider("Timeout Time", &timer.timeout_time, 0.017f, 1.0f, "%.3f");
        }

        ImGui::End();

        rlImGuiEnd();

        /////////////
        EndDrawing();
        /////////////
    }

    rlImGuiShutdown();
    CloseWindow();
}
