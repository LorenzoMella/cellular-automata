// Very simple implementation of Conway's Game of Life using Raylib

// TODO Comparison should be by symbol, not by Color structure, which is only
// useful for drawing, by this point.
// In the future, Colors can be introduced in the logic itself, by generalizing
// update rules. The HSL format is probably better suited for this purpose.

// TODO
// 1. Make a configuration struct for variables to feed into Dear ImGui
// 2. Extract the imgui part into a separate function (or collection of functions)
// 3. Integrate with argparse++ (marketing!)
// 4. Figure out if bitsets make sense to use


#include <bitset>
#include <getopt.h>
#include <vector>

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

std::bitset<9> SurviveRule      {"000001100"};
std::bitset<9> GenerateRule     {"000001000"};

const char *SurviveRuleLabels[9]  {"0##S", "1##S", "2##S", "3##S", "4##S",
                                   "5##S", "6##S", "7##S", "8##S"};
const char *GenerateRuleLabels[9] {"0##G", "1##G", "2##G", "3##G", "4##G",
                                   "5##G", "6##G", "7##G", "8##G"};

Color AliveColor      = BLUE;
Color DeadColor       = WHITE;
Color BackgroundColor = {150, 150, 150, 255};


struct Vec2i
{
    int x;
    int y;
};


Vec2i RelativePairs[] {{-1, -1}, { 0, -1}, { 1, -1},
                       {-1,  0},           { 1,  0},
                       {-1,  1}, { 0,  1}, { 1,  1}};


inline int rem(int a, int b)    // expects b > 0
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


using BoundaryRule = int;


template <typename T>
class Board
{
public:

    Board() {};

    Board(Board const& c) = default;

    Board(T const &init_val, int n_rows, int n_cols, BoundaryRule boundary_rule = 0)
    : m_n_rows {n_rows},
      m_n_cols {n_cols},
      m_boundary_rule {boundary_rule},
      m_arr(n_rows * n_cols)
    {
        m_arr.fill(init_val);
    }

    Board(T const &init_val, T const &alt_val, int n_rows, int n_cols,
          float rate, BoundaryRule boundary_rule = 0)
    : m_n_rows {n_rows},
      m_n_cols {n_cols},
      m_boundary_rule {boundary_rule},
      m_arr(n_rows * n_cols)
    {
        random_fill(rate, init_val, alt_val);
    }

    ~Board() = default;

    T&        get(int i, int j)               { return m_arr[i * m_n_cols + j]; }

    T const & get(int i, int j) const         { return m_arr[i * m_n_cols + j]; }

    T&        set(int i, int j, T const &val) { return (m_arr[i * m_n_cols + j] = val); }

    BoundaryRule& get_boundary_rule()               { return m_boundary_rule; }

    void          set_boundary_rule(BoundaryRule b) { m_boundary_rule = b; }

    int count_neighbors(int i, int j, T const &to, bool (*comp)(T a, T b));

    void random_fill(float rate, T const &init_val, T const &alt_val);

private:

    std::vector<T> m_arr;

    int m_n_rows;
    int m_n_cols;
    BoundaryRule m_boundary_rule;
};


template <typename T>
int Board<T>::count_neighbors(int i, int j, T const &to, bool (*comp)(T a, T b))
{
    int n = 0;
    if (m_boundary_rule == 0) { // thoroidal board

        for (auto d : RelativePairs) {
            n += (int)comp(get(rem(i + d.y, NRows), rem(j + d.x, NCols)), to);
        }

    } else if (m_boundary_rule == 1) { // alive barrier

        for (auto d : RelativePairs) {
            auto neigh_i = i + d.y, neigh_j = j + d.x;
            if (neigh_i < 0 || neigh_i >= NRows || neigh_j < 0 || neigh_j >= NCols) {
                n++;
            } else {
                n += (int)comp(get(i + d.y, j + d.x), to);
            }
        }
    }
    return n;
}


template <typename T>
void Board<T>::random_fill(float rate, T const &init_val, T const &alt_val)
{
    for (size_t i = 0; i < m_arr.size(); ++i) {
        m_arr[i] = (rand() / (float)RAND_MAX) <= rate ? alt_val : init_val;
    }
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
    {"survive-rule",  required_argument, nullptr, 'S'},
    {"generate-rule", required_argument, nullptr, 'G'},
    {0, 0, 0, 0}
};


void parse_options(int argc, char *argv[])
{
    int c;

    for (;;) {
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "W:H:s:r:S:G:", long_options, &option_index);

        TraceLog(LOG_INFO, "%c", c);
        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }

        switch (c)
        {
            case 'W':
                WinWidth = strtol(optarg, NULL, 10);
                break;

            case 'H':
                WinHeight = strtol(optarg, NULL, 10);
                break;

            case 's':
                BlockSide = strtol(optarg, NULL, 10);
                break;

            case 'r':
                FillRate = strtod(optarg, NULL);
                break;

            case 'S':
                SurviveRule = std::bitset<9>{optarg};
                break;

            case 'G':
                GenerateRule = std::bitset<9>{optarg};
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort();
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        TraceLog(LOG_INFO, "non-option ARGV-elements: ");
        while (optind < argc) {
            TraceLog(LOG_INFO, "%s", argv[optind++]);
        }
    }
    NRows = WinHeight / BlockSide;
    NCols = WinWidth / BlockSide;
}


int main(int argc, char *argv[])
{
    parse_options(argc, argv);

    InitWindow(WinWidth, WinHeight, "Game Of Life");
    SetTargetFPS(60);
    rlImGuiSetup(true);

    Board<Color> board1 {DeadColor, AliveColor, NRows, NCols, FillRate};
    Board<Color> board2 {board1};

    Board<Color> *front {&board2}, *back {&board1};

    bool do_update_board = false;

    bool survive_rule_checkboxes[9];
    bool generate_rule_checkboxes[9];

    for (int i = 0; i < 9; ++i) {
        survive_rule_checkboxes[i] = SurviveRule[i];
        generate_rule_checkboxes[i] = GenerateRule[i];
    }

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

                        int n = front->count_neighbors(i, j, AliveColor, ColorIsEqual);

                        if (ColorIsEqual(front->get(i, j), DeadColor)) {

                            if (GenerateRule[n]) {
                                back->set(i, j, AliveColor);
                            } else {
                                back->set(i, j, DeadColor);
                            }

                        } else if (ColorIsEqual(front->get(i, j), AliveColor)) {

                            if (SurviveRule[n]) {
                                back->set(i, j, AliveColor);
                            } else {
                                back->set(i, j, DeadColor);
                            }

                        } else {
                            // Should never happen
                            TraceLog(LOG_INFO, "Neither Alive nor Dead?");
                            back->set(i, j, front->get(i, j));
                        }
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

        if (ImGui::Begin("Settings##game-of-life")) {

            ImGui::Text("Survive when surrounded by...");
            for (int i = 0; i < 9; ++i) {
                if (ImGui::Checkbox(SurviveRuleLabels[i], &survive_rule_checkboxes[i])) {
                    SurviveRule[i] = survive_rule_checkboxes[i];
                }
                if (i < 8) ImGui::SameLine();
            }

            ImGui::Text("Revive when surrounded by...");
            for (int i = 0; i < 9; ++i) {
                if (ImGui::Checkbox(GenerateRuleLabels[i], &generate_rule_checkboxes[i])) {
                    GenerateRule[i] = generate_rule_checkboxes[i];
                }
                if (i < 8) ImGui::SameLine();
            }
            ImGui::Separator();
            if (ImGui::RadioButton("Boundary rule##0", &front->get_boundary_rule(), 0)) {
                board1.set_boundary_rule(0);
                board2.set_boundary_rule(0);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Boundary rule##1", &front->get_boundary_rule(), 1)) {
                board1.set_boundary_rule(1);
                board2.set_boundary_rule(1);
            }

            ImGui::Separator();
            ImGui::SliderFloat("Random Rate", &FillRate, 0.0f, 1.0f, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button("Fill at random")) {
                front->random_fill(FillRate, DeadColor, AliveColor);
            }
            ImGui::Separator();
            ImGui::SliderFloat("Timeout Time", &timer.timeout_time, 0.017f, 1.0f, "%.3f");
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
