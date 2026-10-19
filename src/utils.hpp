#ifndef H_UTILS_HPP
#define H_UTILS_HPP


#include <raylib.h>
#include <cmath>


namespace utils {


struct Vec2i
{
    int x;
    int y;
};


/** Mathematical reminder function */
static inline int rem(int a, int b)
{
    return std::abs((a % b + b) % b);
}


static inline void arr2col(float col3[], Color &c)
{
    c.r = (unsigned char)(col3[0] * 255.0f);
    c.g = (unsigned char)(col3[1] * 255.0f);
    c.b = (unsigned char)(col3[2] * 255.0f);
    c.a = 255;
}


static inline void col2arr(Color &c, float col3[])
{
    col3[0] = (float)c.r / 255.0f;
    col3[1] = (float)c.g / 255.0f;
    col3[2] = (float)c.b / 255.0f;
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


} // namespace utils


#endif /* H_UTILS_HPP */
