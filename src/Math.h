#pragma once
#include <cmath>

struct Vec2{
    float x, y;

};

static inline float math_vec2_dot(Vec2 a, Vec2 b){
    return (float)(a.x * b.x + a.y * b.y);
}

static inline float math_vec2_len(Vec2 v){
    return (float)sqrt(math_vec2_dot(v, v));
}

struct Vec3{
    float x, y, z;
};