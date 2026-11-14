#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
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
    Plant,
    Glass,
    Count
};

const char* materialName(Material material);

enum class ShapeForm : uint8_t { Rect, Circle };
enum class ShapeStyle : uint8_t { Solid, Stroke };

// CPU falling-sand grid. Publishes a straight-alpha RGBA8 buffer for the presenter.
class ParticleSimulator {
public:
    ParticleSimulator(int width, int height);

    void update(float deltaTime);
    void paint(int x, int y, int radius, Material material);
    void paintShape(int x0, int y0, int x1, int y1, ShapeForm form, ShapeStyle style, int stroke, Material material);
    void clear();
    void publish();
    uint8_t* pixelData() { return m_pixels.data(); }
    void materialRgb(Material material, uint8_t& r, uint8_t& g, uint8_t& b) const;

    void setMaterial(Material material) { m_material = material; }
    Material material() const { return m_material; }
    void setRunning(bool running) { m_running = running; }
    bool running() const { return m_running; }
    void cancelIntro() { m_intro = false; }
    void setBrushRadius(int radius);
    int brushRadius() const { return m_brush; }
    void setView(float centerX, float centerY, float zoom);
    float viewX() const { return m_viewX; }
    float viewY() const { return m_viewY; }
    float viewZoom() const { return m_viewZoom; }

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

    struct IntroLetter {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
    };

    void seed();
    void step();
    void playIntro();
    void igniteLetter(const IntroLetter& letter);
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
    void moveCell(int x, int y, int nx, int ny);
    void explode(int cx, int cy);
    void radiate(int x, int y, int power, int reach);
    int nearestHeat(int x, int y) const;
    int smolderDelay(int x, int y) const;
    bool smolder(int x, int y);
    void quench(int x, int y);
    void shove(int cx, int cy, int radius, int lift, int power, bool liquidsOnly);
    bool nudge(int& x, int& y, int cx, int cy, int lift, int distance);
    bool rooted(int x, int y) const;
    bool drinkFromRoot(int x, int y);
    bool growTree(int x, int y);
    void become(Cell& cell, int x, int y, Material material, uint8_t life);
    bool neighborAt(int x, int y, int direction, int& nx, int& ny) const;
    bool inBounds(int x, int y) const;
    bool keepsAwake(const Cell& cell) const;
    void account(int x, int y, Material oldId, Material newId);
    void touch(int x, int y);
    void discardEmptyChunks();
    uint32_t rnd();

    static constexpr int kChunkSize = 64;
    struct ChunkKey {
        int x = 0;
        int y = 0;
        bool operator==(const ChunkKey& other) const { return x == other.x && y == other.y; }
    };
    struct ChunkKeyHash {
        size_t operator()(ChunkKey key) const noexcept {
            const uint64_t x = static_cast<uint32_t>(key.x);
            const uint64_t y = static_cast<uint32_t>(key.y);
            return static_cast<size_t>(x * 0x9E3779B185EBCA87ull ^ (y + 0xC2B2AE3D27D4EB4full + (x << 6) + (x >> 2)));
        }
    };
    struct Chunk {
        Cell cells[kChunkSize * kChunkSize]{};
        uint16_t occupied = 0;
        uint16_t anchors = 0;
        bool awake = true;
        bool dirty = false;
        bool restless = false;
        bool woken = false;
    };

    ChunkKey chunkKey(int x, int y) const;
    int chunkLocal(int x, int y) const;
    Chunk* findChunk(int x, int y);
    const Chunk* findChunk(int x, int y) const;
    const Cell& peek(int x, int y) const;
    Cell& at(int x, int y);
    const Cell& at(int x, int y) const;
    void wakeAround(ChunkKey key);
    bool hasSupport(ChunkKey key) const;
    void cullBelow(int maxChunkY);

    int m_width = 0;
    int m_height = 0;
    int m_brush = 8;
    uint32_t m_frame = 0;
    uint32_t m_rng = 0xA341316Cu;
    float m_time = 0.f;
    bool m_running = true;
    bool m_intro = true;
    bool m_fuseLit = false;
    float m_introTime = 0.f;
    int m_lettersLit = 0;
    int m_letterCount = 0;
    int m_lavaX = 0;
    int m_lavaY = 0;
    int m_fuseX = -1;
    int m_fuseY = -1;
    int m_stageLeft = 0;
    int m_stageTop = 0;
    int m_stageRight = 0;
    int m_stageBottom = 0;
    float m_viewX = 0.f;
    float m_viewY = 0.f;
    float m_viewZoom = 1.f;
    IntroLetter m_letters[16]{};
    Material m_material = Material::Sand;
    Cell m_sink{};
    std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash> m_chunks;
    std::vector<uint8_t> m_pixels;
};
