#pragma once

#include <stdint.h>
#include <stdlib.h>

#define utilities_clamp(V, MIN, MAX) ((V) > (MAX) ? (MAX) : (V) < (MIN) ? (MIN) : (V))

class Utilities
{
private:
    /* data */
public:
    Utilities(/* args */);
    ~Utilities();

    static int32_t random_val(int32_t lower, int32_t upper)
    {
        if (upper < lower) {
            int32_t tmp = lower;
            lower = upper;
            upper = tmp;
        }
        return (rand() % (upper - lower + 1) + lower);
    }
    static inline float interp_linear(float a, float b, float t)
    {
        return a + (b - a) * t;
    }
};

Utilities::Utilities(/* args */)
{
}

Utilities::~Utilities()
{
}
