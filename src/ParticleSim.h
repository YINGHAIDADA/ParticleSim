#pragma once
// main.cpp
#include <SDL3/SDL.h>
#include <stdint.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#include "Math.h"

// 粒子类型定义
enum MaterialType {
    MAT_EMPTY=0x00, 
    MAT_SAND, 
    MAT_WATER, 
    MAT_SALT, 
    MAT_WOOD, 
    MAT_FIRE, 
    MAT_SMOKE, 
    MAT_EMBER, 
    MAT_STEAM,
    MAT_GUNPOWDER, 
    MAT_OIL, 
    MAT_LAVA, 
    MAT_STONE, 
    MAT_ACID
};

struct Color {
    uint8_t r, g, b, a;
};

struct Particle {
    uint8_t id;
    float lifetime;
    Vec2 velocity;
    Color color;
    bool updated;
};


//id
#define mat_id_empty (uint8_t)0
#define mat_id_sand  (uint8_t)1
#define mat_id_water (uint8_t)2
#define mat_id_salt (uint8_t)3
#define mat_id_wood (uint8_t)4
#define mat_id_fire (uint8_t)5
#define mat_id_smoke (uint8_t)6
#define mat_id_ember (uint8_t)7
#define mat_id_steam (uint8_t)8
#define mat_id_gunpowder (uint8_t)9
#define mat_id_oil (uint8_t)10
#define mat_id_lava (uint8_t)11
#define mat_id_stone (uint8_t)12
#define mat_id_acid (uint8_t)13

// Colors
#define mat_col_empty (Color){0, 0, 0, 0}
#define mat_col_sand  (Color){150, 100, 50, 255}
#define mat_col_salt  (Color){200, 180, 190, 255}
#define mat_col_water (Color){20, 100, 170, 200}
#define mat_col_stone (Color){120, 110, 120, 255}
#define mat_col_wood (Color){60, 40, 20, 255}
#define mat_col_fire  (Color){150, 20, 0, 255}
#define mat_col_smoke (Color){50, 50, 50, 255}
#define mat_col_ember (Color){200, 120, 20, 255}
#define mat_col_steam (Color){220, 220, 250, 255}
#define mat_col_gunpowder (Color){60, 60, 60, 255}
#define mat_col_oil (Color){80, 70, 60, 255}
#define mat_col_lava  (Color){200, 50, 0, 255}
#define mat_col_acid  (Color){90, 200, 60, 255}

class ParticleSimulator {
private:
    std::vector<Particle>* m_particles = {0};
    Color* color_buffer = {0};
    
    SDL_Window* m_window;
    int m_textureWidth, m_textureHeight;

    float m_deltaTime;

    int32_t compute_idx(int32_t x, int32_t y)
    {
        return (y * m_textureWidth + x);
    }

    int32_t in_bounds(int32_t x, int32_t y)
    {
        if (x < 0 || x > (m_textureWidth - 1) || y < 0 || y > (m_textureHeight - 1)) return false;
        return true;
    }

    int32_t is_empty(int32_t x, int32_t y)
    {
        return (in_bounds(x, y) && (*m_particles)[compute_idx(x, y)].id == mat_id_empty);
    }

    Particle get_particle_at(int32_t x, int32_t y)
    {
        return (*m_particles)[compute_idx(x, y)];
    }

    void write_data(int32_t idx, Particle p)
    {
        // Write into particle data for id value
        (*m_particles)[idx] = p;
        color_buffer[idx] = p.color;
    }

    Particle particle_empty();
    Particle particle_sand();
    Particle particle_water();
    Particle particle_salt();
    Particle particle_wood();
    Particle particle_fire();
    Particle particle_lava();
    Particle particle_smoke();
    Particle particle_ember();
    Particle particle_steam();
    Particle particle_gunpowder();
    Particle particle_oil();
    Particle particle_stone();
    Particle particle_acid();

    // Particle updates
    void update_particle_sim();
    void update_sand(uint32_t x, uint32_t y);
    void update_water(uint32_t x, uint32_t y);
    void update_salt(uint32_t x, uint32_t y);
    void update_fire(uint32_t x, uint32_t y);
    void update_lava(uint32_t x, uint32_t y);
    void update_smoke(uint32_t x, uint32_t y);
    void update_ember(uint32_t x, uint32_t y);
    void update_steam(uint32_t x, uint32_t y);
    void update_gunpowder(uint32_t x, uint32_t y);
    void update_oil(uint32_t x, uint32_t y);
    void update_acid(uint32_t x, uint32_t y);
    void update_default(uint32_t x, uint32_t y);
    
public:
    ParticleSimulator();
    ParticleSimulator(int texture_wdith, int texture_height);
    ~ParticleSimulator();

    void init();

    void resetParticles();

    void update(float deltaTime);

    float m_gravity = 10.f; // pixels per second per second
    float m_selection_radius = 10.f;
    bool m_show_material_selection_panel = true;
    bool m_run_simulation = true;
    bool m_show_frame_count = true;
    bool m_use_post_processing = true;

};