#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

enum class Material : uint8_t {
    Empty = 0,
    Sand,
    Water,
    Salt,
    Wood,
    Fire,
    Smoke,
    Ember,
    Steam,
    Gunpowder,
    Oil,
    Lava,
    Stone,
    Acid,
    Count
};

const char* materialName(Material material);

// CPU falling-sand grid. Publishes a straight-alpha RGBA8 buffer for the presenter.
class ParticleSimulator {
public:
    ParticleSimulator(int width, int height);

    void update(float deltaTime);
    void paint(int x, int y, int radius, Material material);
    void clear();
    void publish();
    uint8_t* pixelData() { return m_pixels.data(); }
    void materialRgb(Material material, uint8_t& r, uint8_t& g, uint8_t& b) const;

    void setMaterial(Material material) { m_material = material; }
    Material material() const { return m_material; }
    void setRunning(bool running) { m_running = running; }
    bool running() const { return m_running; }
    void setBrushRadius(int radius);
    int brushRadius() const { return m_brush; }

    const uint8_t* pixels() const { return m_pixels.data(); }
    int width() const { return m_width; }
    int height() const { return m_height; }
    size_t pixelBytes() const { return m_pixels.size(); }

private:
    struct Cell {
        Material id = Material::Empty;
        uint8_t shade = 0;
        uint8_t life = 0;
        uint8_t updated = 0;
    };

    void seed();
    void step();
    void fill(int cx, int cy, int radius, Material material);
    void setCell(int x, int y, Material material);
    void updateCell(int x, int y);
    bool react(int x, int y);
    bool trySwap(int x, int y, int nx, int ny);
    bool drop(int x, int y, int maxDistance);
    bool slide(int x, int y, int dx, int maxDistance);
    void movePowder(int x, int y);
    void moveLiquid(int x, int y);
    void moveLava(int x, int y);
    void moveFire(int x, int y);
    void moveEmber(int x, int y);
    void moveSmoke(int x, int y);
    void moveSteam(int x, int y);
    void moveGas(int x, int y);
    void spawn(int x, int y, Material material, uint8_t life);
    void explode(int cx, int cy);
    void become(Cell& cell, Material material, uint8_t life);
    bool neighborAt(int x, int y, int direction, int& nx, int& ny) const;
    bool inBounds(int x, int y) const;
    int index(int x, int y) const;
    uint32_t rnd();

    int m_width = 0;
    int m_height = 0;
    int m_brush = 8;
    uint32_t m_frame = 0;
    uint32_t m_rng = 0xA341316Cu;
    float m_time = 0.f;
    bool m_running = true;
    Material m_material = Material::Sand;
    std::vector<Cell> m_cells;
    std::vector<uint8_t> m_pixels;
};
