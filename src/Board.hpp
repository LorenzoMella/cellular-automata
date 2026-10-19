#ifndef H_BOARD_HPP
#define H_BOARD_HPP


#include "utils.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <raylib.h>
#include <vector>


enum class BoundaryRule
{
    THOROIDAL,
    ALWAYS_ALIVE,
};


template <typename T>
class Board
{
  public:
    Board(T const &init_val, int n_rows, int n_cols, BoundaryRule boundary_rule)
      : Board(n_rows, n_cols, boundary_rule)
    {
        std::fill(m_arr.begin(), m_arr.end(), init_val);
    }

    Board(std::function<T()>alt_val_fn, int n_rows, int n_cols, BoundaryRule boundary_rule)
      : Board(n_rows, n_cols, boundary_rule)
    {
        std::generate(m_arr.begin(), m_arr.end(), alt_val_fn);
    }

    Board(T const &init_val, T const &alt_val, float rate, int n_rows, int n_cols, BoundaryRule boundary_rule)
      : Board([=]() -> T { return (float)std::rand() / RAND_MAX < rate ? alt_val : init_val; },
              n_rows,
              n_cols,
              boundary_rule)
    { }

    Board(Board const &c) = default;
    Board(Board &&c)      = default;
    ~Board()              = default;

    T &get(int i, int j)             { return m_arr[i * m_n_cols + j]; }
    T const &get(int i, int j) const { return m_arr[i * m_n_cols + j]; }

    T &set(int i, int j, T const &val) { return (m_arr[i * m_n_cols + j] = val); }

    int count_neighbors(int i, int j, T const &to, bool (*comp)(T a, T b));

    void fill(T const &val) { std::fill(m_arr.begin(), m_arr.end(), val); };

    void random_fill(float rate, T const &init_val, T const &alt_val);

    void random_fill(std::function<T()> generator_fn);

    void draw(std::function<Color(T const &)>color_map, Vector2 block_size) const;

  private:
    Board(int n_rows, int n_cols, BoundaryRule boundary_rule)
      : m_boundary_rule {boundary_rule},
        m_arr(n_rows * n_cols),
        m_n_rows {n_rows},
        m_n_cols {n_cols}
    { }

  public:
    BoundaryRule   m_boundary_rule;

  private:
    std::vector<T> m_arr;
    int            m_n_rows;
    int            m_n_cols;
};


// clang-format off
static utils::Vec2i const RelativePairs[] {
    {-1, -1}, { 0, -1}, { 1, -1},
    {-1,  0},           { 1,  0},
    {-1,  1}, { 0,  1}, { 1,  1},
};
// clang-format on


template <typename T>
int Board<T>::count_neighbors(int i, int j, T const &to, bool (*comp)(T a, T b))
{
    int n = 0;
    if (m_boundary_rule == BoundaryRule::THOROIDAL)
    {
        for (auto d : RelativePairs)
        {
            n += (int)comp(get(utils::rem(i + d.y, m_n_rows), utils::rem(j + d.x, m_n_cols)), to);
        }
    }
    else if (m_boundary_rule == BoundaryRule::ALWAYS_ALIVE)
    {
        for (auto d : RelativePairs)
        {
            auto neigh_i = i + d.y, neigh_j = j + d.x;
            if (neigh_i < 0 || neigh_i >= m_n_rows || neigh_j < 0 || neigh_j >= m_n_cols)
            {
                n++;
            }
            else
            {
                n += (int)comp(get(i + d.y, j + d.x), to);
            }
        }
    }
    return n;
}


template <typename T>
void Board<T>::draw(std::function<Color(T const &)>color_map, Vector2 block_size) const
{
    Vector2 size {block_size.x - 2.0f, block_size.y - 2.0f};
    Vector2 position {1.0f, 1.0f};

    for (int i = 0; i < m_n_rows; ++i)
    {
        for (int j = 0; j < m_n_cols; ++j)
        {
            DrawRectangleV(position, size, color_map(get(i, j)));
            position.x += block_size.x;
        }
        position.x = 1;
        position.y += block_size.y;
    }
}


template <typename T>
void Board<T>::random_fill(std::function<T()> generator_fn)
{
    std::generate(m_arr.begin(), m_arr.end(), generator_fn);
}


template <typename T>
void Board<T>::random_fill(float rate, T const &init_val, T const &alt_val)
{
    random_fill([=] () { return ((float)std::rand() / RAND_MAX < rate ? alt_val : init_val); });
}


#endif /* H_BOARD_HPP */
