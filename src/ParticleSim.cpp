#include "ParticleSim.h"

#include "Hud.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

enum class Phase : uint8_t { Empty, Static, Powder, Liquid, Gas };

struct MaterialInfo {
    Phase phase;
    uint8_t density;
    uint8_t r, g, b, a;
    uint8_t jitter;
};

constexpr MaterialInfo kMaterials[] = {
    {Phase::Empty, 0, 0, 0, 0, 0, 0},
    {Phase::Powder, 4, 150, 100, 50, 255, 18},
    {Phase::Liquid, 3, 20, 100, 170, 200, 8},
    {Phase::Powder, 4, 200, 180, 190, 255, 8},
    {Phase::Static, 9, 60, 40, 20, 255, 8},
    {Phase::Gas, 1, 150, 20, 0, 255, 0},
    {Phase::Gas, 1, 50, 50, 50, 180, 6},
    {Phase::Powder, 4, 200, 120, 20, 255, 10},
    {Phase::Gas, 1, 220, 220, 250, 160, 4},
    {Phase::Powder, 4, 60, 60, 60, 255, 6},
    {Phase::Liquid, 2, 80, 70, 60, 230, 6},
    {Phase::Liquid, 6, 200, 50, 0, 255, 10},
    {Phase::Static, 9, 120, 110, 120, 255, 8},
    {Phase::Liquid, 3, 90, 200, 60, 200, 8},
    {Phase::Static, 9, 46, 150, 42, 255, 20},
    {Phase::Static, 9, 214, 232, 242, 120, 8},
};

static_assert(std::size(kMaterials) == static_cast<size_t>(Material::Count));

const MaterialInfo& infoOf(Material material) {
    const auto id = static_cast<size_t>(material);
    if (id >= static_cast<size_t>(Material::Count)) {
        throw std::logic_error("material id out of range");
    }
    return kMaterials[id];
}

int vary(uint8_t base, uint8_t shade, uint8_t amount) {
    if (amount == 0) return base;
    const int delta = (int(shade) - 128) * int(amount) / 128;
    return std::clamp(int(base) + delta, 0, 255);
}

bool flungByBlast(Material material, bool liquidsOnly) {
    switch (material) {
    case Material::Water:
    case Material::Oil:
    case Material::Acid:
        return true;
    case Material::Sand:
    case Material::Salt:
    case Material::Gunpowder:
    case Material::Ember:
    case Material::Lava:
        return !liquidsOnly;
    case Material::Empty:
    case Material::Wood:
    case Material::Fire:
    case Material::Smoke:
    case Material::Steam:
    case Material::Stone:
    case Material::Plant:
    case Material::Glass:
    case Material::Count:
        return false;
    }
    const Material unhandled = material;
    throw std::logic_error("unhandled material " + std::to_string(int(unhandled)));
}

bool dissolves(Material material) {
    switch (material) {
    case Material::Sand:
    case Material::Salt:
    case Material::Wood:
    case Material::Gunpowder:
    case Material::Oil:
    case Material::Ember:
    case Material::Plant:
        return true;
    case Material::Empty:
    case Material::Water:
    case Material::Fire:
    case Material::Smoke:
    case Material::Steam:
    case Material::Lava:
    case Material::Stone:
    case Material::Acid:
    case Material::Glass:
    case Material::Count:
        return false;
    }
    const Material unhandled = material;
    throw std::logic_error("unhandled material " + std::to_string(int(unhandled)));
}

uint8_t lifeFor(Material material, uint32_t noise) {
    switch (material) {
    case Material::Fire:
        return uint8_t(130 + (noise % 40));
    case Material::Smoke:
        return uint8_t(100 + (noise % 80));
    case Material::Steam:
        return uint8_t(70 + (noise % 50));
    case Material::Ember:
        return uint8_t(50 + (noise % 40));
    case Material::Empty:
    case Material::Sand:
    case Material::Water:
    case Material::Salt:
    case Material::Wood:
    case Material::Gunpowder:
    case Material::Oil:
    case Material::Lava:
    case Material::Stone:
    case Material::Acid:
    case Material::Plant:
    case Material::Glass:
    case Material::Count:
        return 0;
    }
    const Material unhandled = material;
    throw std::logic_error("unhandled material " + std::to_string(int(unhandled)));
}

bool usesLife(Material material) {
    switch (material) {
    case Material::Fire:
    case Material::Smoke:
    case Material::Steam:
    case Material::Ember:
        return true;
    case Material::Empty:
    case Material::Sand:
    case Material::Water:
    case Material::Salt:
    case Material::Wood:
    case Material::Gunpowder:
    case Material::Oil:
    case Material::Lava:
    case Material::Stone:
    case Material::Acid:
    case Material::Plant:
    case Material::Glass:
    case Material::Count:
        return false;
    }
    const Material unhandled = material;
    throw std::logic_error("unhandled material " + std::to_string(int(unhandled)));
}

}  // namespace

const char* materialName(Material material) {
    switch (material) {
    case Material::Empty: return "Empty";
    case Material::Sand: return "Sand";
    case Material::Water: return "Water";
    case Material::Salt: return "Salt";
    case Material::Wood: return "Wood";
    case Material::Fire: return "Fire";
    case Material::Smoke: return "Smoke";
    case Material::Ember: return "Ember";
    case Material::Steam: return "Steam";
    case Material::Gunpowder: return "Gunpowder";
    case Material::Oil: return "Oil";
    case Material::Lava: return "Lava";
    case Material::Stone: return "Stone";
    case Material::Acid: return "Acid";
    case Material::Plant: return "Plant";
    case Material::Glass: return "Glass";
    case Material::Count: return "Unknown";
    }
    const Material unhandled = material;
    throw std::logic_error("unhandled material " + std::to_string(int(unhandled)));
}

ParticleSimulator::ParticleSimulator(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("simulation size must be positive");
    }
    m_width = width;
    m_height = height;
    const size_t cells = static_cast<size_t>(width) * static_cast<size_t>(height);
    m_pixels.resize(cells * 4);
    seed();
}

void ParticleSimulator::setBrushRadius(int radius) {
    m_brush = std::clamp(radius, 1, 48);
}

void ParticleSimulator::update(float deltaTime) {
    if (!m_running) return;
    if (m_intro) m_introTime += deltaTime;
    m_time += deltaTime;
    constexpr float kStep = 1.f / 60.f;
    if (m_time < kStep) return;
    playIntro();
    step();
    m_time = std::min(m_time - kStep, kStep);
}

void ParticleSimulator::paint(int x, int y, int radius, Material material) {
    fill(x, y, radius, material);
    discardEmptyChunks();
}

void ParticleSimulator::paintShape(
    int x0,
    int y0,
    int x1,
    int y1,
    ShapeForm form,
    ShapeStyle style,
    int stroke,
    Material material
) {
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
    switch (form) {
    case ShapeForm::Rect:
        minX = std::min(x0, x1);
        maxX = std::max(x0, x1);
        minY = std::min(y0, y1);
        maxY = std::max(y0, y1);
        break;
    case ShapeForm::Circle: {
        const int rdx = x1 - x0;
        const int rdy = y1 - y0;
        const int reach = int(std::ceil(std::sqrt(double(rdx * rdx + rdy * rdy)) - 1e-9));
        minX = x0 - reach;
        maxX = x0 + reach;
        minY = y0 - reach;
        maxY = y0 + reach;
        break;
    }
    }

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (!shapeContains(x, y, x0, y0, x1, y1, form, style, stroke)) continue;
            setCell(x, y, material);
        }
    }
    discardEmptyChunks();
}

void ParticleSimulator::materialRgb(Material material, uint8_t& r, uint8_t& g, uint8_t& b) const {
    if (material == Material::Fire) {
        r = 220;
        g = 70;
        b = 16;
        return;
    }
    const MaterialInfo& info = infoOf(material);
    r = info.r;
    g = info.g;
    b = info.b;
}

void ParticleSimulator::setView(float centerX, float centerY, float zoom) {
    m_viewX = centerX;
    m_viewY = centerY;
    m_viewZoom = std::clamp(zoom, 0.2f, 16.f);
}

void ParticleSimulator::clear() {
    m_intro = false;
    m_chunks.clear();
    publish();
}

void ParticleSimulator::seed() {
    constexpr const char* kTitle = "particleSim";
    constexpr int kGap = 2;
    constexpr int kThickness = 2;
    const int scale = 6;
    auto titleWidth = [](int glyphScale) {
        constexpr int kLetters = 11;
        return kLetters * (5 * glyphScale + kGap) - kGap;
    };
    const int textWidth = titleWidth(scale);
    const int textHeight = 7 * scale;
    const int side = std::max(textWidth + 140, textHeight + 280);
    const int innerLeft = 80;
    const int innerTop = 80;
    const int innerRight = innerLeft + side - 1;
    const int innerBottom = innerTop + side - 1;
    m_stageLeft = innerLeft - kThickness;
    m_stageTop = innerTop - kThickness;
    m_stageRight = innerRight + kThickness;
    m_stageBottom = innerBottom + kThickness;

    for (int thickness = 0; thickness < kThickness; ++thickness) {
        const int left = m_stageLeft + thickness;
        const int right = m_stageRight - thickness;
        const int top = m_stageTop + thickness;
        const int bottom = m_stageBottom - thickness;
        for (int x = left; x <= right; ++x) {
            setCell(x, top, Material::Stone);
            setCell(x, bottom, Material::Stone);
        }
        for (int y = top + 1; y < bottom; ++y) {
            setCell(left, y, Material::Stone);
            setCell(right, y, Material::Stone);
        }
    }

    int cursorX = innerLeft + (side - textWidth) / 2;
    const int floorTop = innerBottom + 1;
    const int wallTop = floorTop - std::max(44, scale * 8);
    const int waterBottomLimit = wallTop - 28;
    const int waterHeight = std::max(36, scale * 7);
    const int waterTop = waterBottomLimit - waterHeight;
    int originY = waterTop - 14 - textHeight;
    if (originY < innerTop + 16) originY = innerTop + 16;

    m_letterCount = 0;
    for (int i = 0; kTitle[i] != '\0' && m_letterCount < 16; ++i) {
        const uint8_t* rows = hudGlyphRows(kTitle[i]);
        IntroLetter& letter = m_letters[m_letterCount];
        letter = {cursorX, originY, 5 * scale, textHeight};
        if (rows != nullptr) {
            for (int row = 0; row < 7; ++row) {
                for (int col = 0; col < 5; ++col) {
                    if ((rows[row] & (0x10 >> col)) == 0) continue;
                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            const int px = cursorX + col * scale + sx;
                            const int py = originY + row * scale + sy;
                            if (inBounds(px, py)) setCell(px, py, Material::Wood);
                        }
                    }
                }
            }
            ++m_letterCount;
        }
        cursorX += 5 * scale + kGap;
    }

    const int center = (innerLeft + innerRight) / 2;
    const int basinHalf = std::max(34, scale * 7);
    const int wallThickness = 4;
    const int basinLeft = center - basinHalf;
    const int basinRight = center + basinHalf;
    for (int y = wallTop; y < floorTop; ++y) {
        for (int thickness = 0; thickness < wallThickness; ++thickness) {
            const int left = basinLeft - 1 - thickness;
            const int right = basinRight + 1 + thickness;
            if (inBounds(left, y)) setCell(left, y, Material::Stone);
            if (inBounds(right, y)) setCell(right, y, Material::Stone);
        }
    }

    const int waterHalf = std::max(16, basinHalf / 2);
    const int waterBottom = waterTop + waterHeight;
    for (int y = waterTop; y <= waterBottom; ++y) {
        for (int x = center - waterHalf; x <= center + waterHalf; ++x) {
            if (inBounds(x, y)) setCell(x, y, Material::Water);
        }
    }
    m_lavaX = center;
    m_lavaY = std::min(originY + textHeight + 6, waterTop - 4);

    const int powderRadius = std::clamp(scale + 1, 4, 7);
    int powderX = cursorX + powderRadius + 4;
    int powderY = originY + textHeight / 2;
    if (powderX + powderRadius > innerRight - 2) {
        powderX = innerRight - powderRadius - 2;
    }
    if (powderY > waterTop - powderRadius) powderY = std::max(2, waterTop - powderRadius - 1);
    fill(powderX, powderY, powderRadius, Material::Gunpowder);
    m_fuseX = powderX - powderRadius - 1;
    m_fuseY = powderY;
    if (!inBounds(m_fuseX, m_fuseY)) {
        m_fuseX = -1;
        m_fuseY = -1;
    }

    const float worldW = float(m_stageRight - m_stageLeft + 1);
    const float worldH = float(m_stageBottom - m_stageTop + 1);
    const float canvasW = float(std::max(m_width - hudWidth(), 1));
    const float canvasH = float(std::max(m_height - toolbarHeight(), 1));
    m_viewX = (m_stageLeft + m_stageRight) * 0.5f;
    m_viewY = (m_stageTop + m_stageBottom) * 0.5f;
    m_viewZoom = std::clamp(
        std::min(canvasW / (worldW * 1.45f), canvasH / (worldH * 1.45f)),
        0.2f,
        8.f
    );
    publish();
}

void ParticleSimulator::igniteLetter(const IntroLetter& letter) {
    for (int y = letter.y; y < letter.y + letter.h; ++y) {
        for (int x = letter.x; x < letter.x + letter.w; ++x) {
            if (!inBounds(x, y)) continue;
            if (peek(x, y).id == Material::Wood) setCell(x, y, Material::Fire);
        }
    }
}

void ParticleSimulator::playIntro() {
    if (!m_intro) return;

    constexpr float kLetterStart = 0.45f;
    constexpr float kLetterStep = 0.28f;
    if (m_lettersLit < m_letterCount &&
        m_introTime + 0.16f >= kLetterStart + float(m_lettersLit) * kLetterStep) {
        const IntroLetter& letter = m_letters[m_lettersLit];
        for (int y = letter.y; y < letter.y + letter.h; ++y) {
            for (int x = letter.x; x < letter.x + letter.w; ++x) {
                if (!inBounds(x, y) || peek(x, y).id != Material::Wood) continue;
                Cell& cell = at(x, y);
                if (cell.life < 12) cell.life = 12;
            }
        }
    }
    while (m_lettersLit < m_letterCount &&
           m_introTime >= kLetterStart + float(m_lettersLit) * kLetterStep) {
        igniteLetter(m_letters[m_lettersLit]);
        ++m_lettersLit;
    }

    if (m_introTime >= 2.25f && m_introTime <= 3.25f && (m_frame & 1u) == 0) {
        fill(m_lavaX, m_lavaY, 2, Material::Lava);
    }

    if (!m_fuseLit && m_introTime >= 3.7f && m_fuseX >= 0) {
        setCell(m_fuseX, m_fuseY, Material::Fire);
        m_fuseLit = true;
    }

    if (m_introTime >= 5.4f) m_intro = false;
}

void ParticleSimulator::step() {
    ++m_frame;
    const float zoom = std::max(m_viewZoom, 0.05f);
    const float canvasW = float(std::max(m_width - hudWidth(), 1));
    const float canvasH = float(std::max(m_height - toolbarHeight(), 1));
    const float halfW = canvasW / zoom * 0.5f;
    const float halfH = canvasH / zoom * 0.5f;
    const ChunkKey lo = chunkKey(
        int(std::floor(double(m_viewX - halfW))),
        int(std::floor(double(m_viewY - halfH)))
    );
    const ChunkKey hi = chunkKey(
        int(std::floor(double(m_viewX + halfW))),
        int(std::floor(double(m_viewY + halfH)))
    );
    constexpr int kMargin = 2;
    const int minX = lo.x - kMargin;
    const int minY = lo.y - kMargin;
    const int maxX = hi.x + kMargin;
    const int maxY = hi.y + kMargin;

    std::vector<ChunkKey> keys;
    keys.reserve(m_chunks.size());
    for (auto& entry : m_chunks) {
        const ChunkKey key = entry.first;
        if (key.x < minX || key.x > maxX || key.y < minY || key.y > maxY) continue;
        if (!entry.second->awake) continue;
        keys.push_back(key);
        for (Cell& cell : entry.second->cells) cell.updated = 0;
    }

    const int direction = (m_frame & 1u) ? 1 : -1;
    std::sort(keys.begin(), keys.end(), [direction](ChunkKey a, ChunkKey b) {
        if (a.y != b.y) return a.y > b.y;
        return direction > 0 ? a.x < b.x : a.x > b.x;
    });

    for (const ChunkKey key : keys) {
        const auto found = m_chunks.find(key);
        if (found == m_chunks.end() || !found->second) continue;
        Chunk* chunk = found->second.get();
        chunk->dirty = false;
        chunk->restless = false;
        const int x0 = key.x * kChunkSize;
        const int y0 = key.y * kChunkSize;
        for (int localY = kChunkSize - 1; localY >= 0; --localY) {
            const int y = y0 + localY;
            const int xStart = direction > 0 ? x0 : x0 + kChunkSize - 1;
            const int xEnd = direction > 0 ? x0 + kChunkSize : x0 - 1;
            for (int x = xStart; x != xEnd; x += direction) {
                Cell& cell = chunk->cells[localY * kChunkSize + (x - x0)];
                if (cell.id == Material::Empty) continue;
                if (keepsAwake(cell)) chunk->restless = true;
                updateCell(x, y);
                if (keepsAwake(cell)) chunk->restless = true;
            }
        }
    }

    std::vector<ChunkKey> halos;
    halos.reserve(keys.size());
    for (const ChunkKey key : keys) {
        const auto found = m_chunks.find(key);
        if (found == m_chunks.end() || !found->second) continue;
        if (found->second->dirty || found->second->restless) halos.push_back(key);
    }
    for (const ChunkKey key : halos) wakeAround(key);

    for (const ChunkKey key : keys) {
        const auto found = m_chunks.find(key);
        if (found == m_chunks.end() || !found->second) continue;
        Chunk& chunk = *found->second;
        chunk.awake = chunk.dirty || chunk.restless || chunk.woken;
        chunk.dirty = false;
        chunk.restless = false;
        chunk.woken = false;
    }

    cullBelow(maxY);
    discardEmptyChunks();
}

void ParticleSimulator::publish() {
    const float zoom = std::max(m_viewZoom, 0.05f);
    const int hud = hudWidth();
    const int bar = toolbarHeight();
    const float canvasW = float(std::max(m_width - hud, 1));
    const float canvasH = float(std::max(m_height - bar, 1));
    const float viewW = canvasW / zoom;
    const float viewH = canvasH / zoom;
    const Chunk* cached = nullptr;
    int cacheX = 0;
    int cacheY = 0;
    bool cacheValid = false;
    uint8_t* out = m_pixels.data();
    for (int py = 0; py < m_height; ++py) {
        for (int px = 0; px < m_width; ++px) {
            Cell cell{};
            if (px >= hud && py >= bar) {
                const float localX = (float(px - hud) + 0.5f) / canvasW;
                const float localY = (float(py - bar) + 0.5f) / canvasH;
                const int wx = int(std::floor(double(m_viewX) + (double(localX) - 0.5) * double(viewW)));
                const int wy = int(std::floor(double(m_viewY) + (double(localY) - 0.5) * double(viewH)));
                const ChunkKey key = chunkKey(wx, wy);
                if (!cacheValid || key.x != cacheX || key.y != cacheY) {
                    cacheValid = true;
                    cacheX = key.x;
                    cacheY = key.y;
                    const auto found = m_chunks.find(key);
                    cached = found == m_chunks.end() ? nullptr : found->second.get();
                }
                if (cached != nullptr && inBounds(wx, wy)) cell = cached->cells[chunkLocal(wx, wy)];
            }
        if (cell.id == Material::Empty || cell.id == Material::Count) {
            out[0] = out[1] = out[2] = out[3] = 0;
            out += 4;
            continue;
        }

        const int flicker = int((cell.shade + m_frame * 13u) & 31u) - 16;
        const MaterialInfo& info = infoOf(cell.id);
        int r = vary(info.r, cell.shade, info.jitter);
        int g = vary(info.g, cell.shade, info.jitter);
        int b = vary(info.b, cell.shade, info.jitter);
        int alpha = info.a;

        if (cell.id == Material::Fire) {
            if (cell.life > 120) {
                r = 255;
                g = std::clamp(245 + flicker, 0, 255);
                b = 48;
            } else if (cell.life > 80) {
                r = 255;
                g = std::clamp(190 + flicker, 0, 255);
                b = 16;
            } else if (cell.life > 40) {
                r = 255;
                g = std::clamp(100 + flicker, 0, 255);
                b = 6;
            } else {
                r = std::clamp(210 + flicker, 0, 255);
                g = 36;
                b = 0;
            }
            alpha = 255;
        } else if (cell.id == Material::Ember) {
            r = 255;
            g = std::clamp(70 + int(cell.life) + flicker / 2, 0, 255);
            b = 8;
            alpha = 255;
        } else if (cell.id == Material::Smoke) {
            r = g = b = std::clamp(22 + int(cell.shade) / 16, 0, 48);
            alpha = std::clamp(12 + int(cell.life) / 2, 0, 140);
        } else if (cell.id == Material::Steam) {
            r = 232;
            g = 238;
            b = 255;
            alpha = std::clamp(30 + int(cell.life), 0, 200);
        } else if (cell.id == Material::Plant && cell.shade >= 200) {
            r = 118;
            g = std::clamp(206 + flicker / 3, 0, 255);
            b = 62;
            if (cell.life > 0) {
                const int charred = std::min(int(cell.life) * 180 / 54, 180);
                r = std::min(255, r + charred);
                g = std::max(0, g - charred / 3);
                b = std::max(0, b - charred / 2);
            }
        } else if (cell.id == Material::Sand && cell.life > 0) {
            const int bake = std::min(int(cell.life), 120);
            r = r + (230 - r) * bake / 120;
            g = g + (242 - g) * bake / 120;
            b = b + (248 - b) * bake / 120;
        } else if (cell.id == Material::Glass) {
            alpha = 100 + int(cell.shade % 36);
        } else if ((cell.id == Material::Wood || cell.id == Material::Plant) && cell.life > 0) {
            const int scale = cell.id == Material::Plant ? 54 : 36;
            const int charred = std::min(int(cell.life) * 180 / scale, 180);
            r = std::min(255, r + charred);
            g = std::max(0, g - charred / 3);
            b = std::max(0, b - charred / 2);
        } else if (cell.id == Material::Oil && cell.life > 0) {
            const int heat = std::min(255, int(cell.life) * 5);
            r = r + (255 - r) * heat / 255;
            g = g + (150 - g) * heat / 255;
            b = b + (16 - b) * heat / 255;
        } else if (cell.id == Material::Salt && cell.life > 0) {
            alpha = std::max(40, 255 - int(cell.life) * 40);
        } else if (cell.id == Material::Water && cell.life > 0) {
            const int cloud = std::min(int(cell.life), 120);
            r = r + (232 - r) * cloud / 120;
            g = g + (238 - g) * cloud / 120;
            b = b + (255 - b) * cloud / 120;
        } else if (cell.id == Material::Acid && cell.life > 0) {
            const int stain = std::min(int(cell.life), 40);
            r = r + (int(cell.shade) - r) * stain / 80;
            g = g + (80 - g) * stain / 160;
        } else if (cell.id == Material::Stone && cell.life > 0) {
            const int heat = std::min(int(cell.life), 220);
            r = r + (255 - r) * heat / 220;
            g = g + (90 - g) * heat / 280;
            b = b * (220 - heat) / 220;
        } else if (cell.id == Material::Lava) {
            g = std::clamp(g + flicker, 20, 120);
            r = 255;
            b = 0;
        }

        out[0] = uint8_t(std::clamp(r, 0, 255));
        out[1] = uint8_t(std::clamp(g, 0, 255));
        out[2] = uint8_t(std::clamp(b, 0, 255));
        out[3] = uint8_t(std::clamp(alpha, 0, 255));
        out += 4;
        }
    }
}

void ParticleSimulator::fill(int cx, int cy, int radius, Material material) {
    if (radius < 0) return;
    const int radiusSq = radius * radius;
    for (int y = cy - radius; y <= cy + radius; ++y) {
        for (int x = cx - radius; x <= cx + radius; ++x) {
            const int dx = x - cx;
            const int dy = y - cy;
            if (dx * dx + dy * dy > radiusSq) continue;
            if (!inBounds(x, y)) continue;
            setCell(x, y, material);
        }
    }
}

void ParticleSimulator::setCell(int x, int y, Material material) {
    if (!inBounds(x, y)) return;
    if (material == Material::Empty && peek(x, y).id == Material::Empty) return;
    Cell& cell = at(x, y);
    const Material old = cell.id;
    cell.id = material;
    cell.shade = material == Material::Plant ? uint8_t(rnd() % 150u) : uint8_t(rnd() & 255u);
    cell.updated = 0;
    cell.life = lifeFor(material, rnd());
    account(x, y, old, material);
}

void ParticleSimulator::updateCell(int x, int y) {
    const Cell& current = at(x, y);
    if (current.id == Material::Empty || current.updated) return;
    if (react(x, y)) return;

    const Cell& cell = at(x, y);
    if (cell.id == Material::Empty || cell.updated) return;
    if (cell.id == Material::Lava) {
        moveLava(x, y);
        return;
    }
    if (cell.id == Material::Fire) {
        moveFire(x, y);
        return;
    }
    if (cell.id == Material::Ember) {
        moveEmber(x, y);
        return;
    }
    if (cell.id == Material::Smoke) {
        moveSmoke(x, y);
        return;
    }
    if (cell.id == Material::Steam) {
        moveSteam(x, y);
        return;
    }
    if (cell.id == Material::Plant) {
        if (!rooted(x, y)) {
            if (!drop(x, y, 3)) {
                const int direction = (rnd() & 1u) ? 1 : -1;
                const int nx = x + direction;
                const int ny = y + 1;
                if (inBounds(nx, ny) && peek(nx, ny).id == Material::Empty) {
                    trySwap(x, y, nx, ny);
                }
            }
        }
        return;
    }

    switch (infoOf(cell.id).phase) {
    case Phase::Empty:
    case Phase::Static:
        return;
    case Phase::Powder:
        movePowder(x, y);
        return;
    case Phase::Liquid:
        moveLiquid(x, y);
        return;
    case Phase::Gas:
        moveGas(x, y);
        return;
    }
    const Phase unhandled = infoOf(cell.id).phase;
    throw std::logic_error("unhandled phase " + std::to_string(int(unhandled)));
}

bool ParticleSimulator::react(int x, int y) {
    Cell& self = at(x, y);
    if (self.id == Material::Water && self.life > 0 && (m_frame & 7u) == 0) {
        --self.life;
    }
    if (self.id == Material::Sand && self.life > 0 && (m_frame & 3u) == 0) {
        --self.life;
    }
    if (self.id == Material::Acid && self.life > 0 && (rnd() & 3u) == 0) {
        --self.life;
    }

    if (self.id == Material::Steam && self.life <= 24) {
        int mateX[2] = {};
        int mateY[2] = {};
        int found = 0;
        for (int dy = -1; dy <= 1 && found < 2; ++dy) {
            for (int dx = -1; dx <= 1 && found < 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                const int nx = x + dx;
                const int ny = y + dy;
                if (!inBounds(nx, ny)) continue;
                const Cell& other = peek(nx, ny);
                if (other.id != Material::Steam || other.life > 24) continue;
                mateX[found] = nx;
                mateY[found] = ny;
                ++found;
            }
        }
        if (found == 2) {
            become(at(mateX[0], mateY[0]), mateX[0], mateY[0], Material::Empty, 0);
            become(at(mateX[1], mateY[1]), mateX[1], mateY[1], Material::Empty, 0);
            become(self, x, y, Material::Water, 0);
            return true;
        }
    }

    if (usesLife(self.id)) {
        if (self.life == 0) {
            if (self.id == Material::Fire) {
                become(self, x, y, Material::Smoke, uint8_t(50 + (rnd() % 50u)));
            } else if (self.id == Material::Ember && (rnd() & 1u) == 0) {
                become(self, x, y, Material::Smoke, uint8_t(30 + (rnd() % 30u)));
            } else {
                become(self, x, y, Material::Empty, 0);
            }
            return true;
        }
        --self.life;
        if (self.id == Material::Fire && self.life < 20 && (rnd() % 5u) == 0) {
            become(self, x, y, Material::Ember, uint8_t(24 + (rnd() % 20u)));
            return true;
        }
    }

    if (self.id == Material::Stone && self.life > 0) {
        if ((m_frame & 1u) == 0 && self.life > 0) --self.life;
        radiate(x, y, std::max(1, int(self.life) / 40), 5);
    }

    if (self.id == Material::Water) {
        for (int i = 0; i < 4; ++i) {
            int nx = 0;
            int ny = 0;
            if (!neighborAt(x, y, i, nx, ny)) continue;
            const Material neighbor = peek(nx, ny).id;
            if (neighbor != Material::Lava && neighbor != Material::Fire) continue;
            Cell& other = at(nx, ny);
            if (other.id == Material::Lava) {
                become(self, x, y, Material::Steam, uint8_t(100 + (rnd() % 40u)));
                spawn(x, y - 1, Material::Steam, uint8_t(80 + (rnd() % 30u)));
                spawn(x - 1, y - 1, Material::Steam, uint8_t(60 + (rnd() % 30u)));
                spawn(x + 1, y - 1, Material::Steam, uint8_t(60 + (rnd() % 30u)));
                spawn(x, y - 2, Material::Steam, uint8_t(40 + (rnd() % 20u)));
                become(other, nx, ny, Material::Stone, 220);
                spawn(nx - 1, ny, Material::Ember, uint8_t(20 + (rnd() % 16u)));
                spawn(nx + 1, ny, Material::Ember, uint8_t(20 + (rnd() % 16u)));
                shove(x, y, 3, 4, 1, true);
                return true;
            }
            if (other.id == Material::Fire) {
                become(self, x, y, Material::Steam, uint8_t(80 + (rnd() % 40u)));
                spawn(x, y - 1, Material::Steam, uint8_t(50 + (rnd() % 30u)));
                spawn(x + ((rnd() & 1u) ? 1 : -1), y - 1, Material::Steam, uint8_t(40 + (rnd() % 20u)));
                become(other, nx, ny, Material::Ember, uint8_t(28 + (rnd() % 16u)));
                spawn(nx, ny - 1, Material::Ember, uint8_t(18 + (rnd() % 12u)));
                return true;
            }
        }
        const int closest = nearestHeat(x, y);
        if (closest <= 6 && (m_frame % uint32_t(std::max(1, closest - 1))) == 0 && self.life < 120) {
            ++self.life;
        }
        if (self.life >= 120) {
            become(self, x, y, Material::Steam, uint8_t(70 + (rnd() % 40u)));
            spawn(x, y - 1, Material::Steam, uint8_t(36 + (rnd() % 24u)));
            return true;
        }
    }

    if (self.id == Material::Salt) {
        for (int i = 0; i < 4; ++i) {
            int nx = 0;
            int ny = 0;
            if (!neighborAt(x, y, i, nx, ny)) continue;
            const Cell& waterPeek = peek(nx, ny);
            if (waterPeek.id != Material::Water || (rnd() % 10u) != 0) continue;
            Cell& water = at(nx, ny);
            self.life = uint8_t(std::min(255, int(self.life) + 1));
            water.life = uint8_t(std::min(80, int(water.life) + 28));
            if (self.life >= 5) {
                spawn(x, y - 1, Material::Steam, uint8_t(16 + (rnd() % 10u)));
                become(self, x, y, Material::Empty, 0);
                return true;
            }
        }
    }

    if (self.id == Material::Acid) {
        int nx = 0;
        int ny = 0;
        if (neighborAt(x, y, int(rnd() % 4u), nx, ny)) {
            const Cell& probed = peek(nx, ny);
            if (dissolves(probed.id) && (rnd() % 3u) == 0) {
            Cell& other = at(nx, ny);
                self.shade = infoOf(other.id).r;
                self.life = 40;
                become(other, nx, ny, Material::Steam, uint8_t(14 + (rnd() % 12u)));
                spawn(nx, ny - 1, Material::Steam, uint8_t(10 + (rnd() % 8u)));
                if ((rnd() % 6u) == 0) {
                    become(self, x, y, Material::Empty, 0);
                    return true;
                }
            }
        }
    }

    if (self.id == Material::Wood) {
        if (smolder(x, y)) return true;
    }

    if (self.id == Material::Wood && self.life == 0 && y > 0) {
        const Cell& abovePeek = peek(x, y - 1);
        if (abovePeek.id == Material::Water && abovePeek.life == 0 && (rnd() % 10u) == 0) {
            Cell& above = at(x, y - 1);
            become(above, x, y - 1, Material::Plant, 0);
            above.shade = uint8_t(52 + (rnd() % 36u));
        } else if (abovePeek.id == Material::Empty && (rnd() % 28u) == 0 && drinkFromRoot(x, y)) {
            Cell& above = at(x, y - 1);
            become(above, x, y - 1, Material::Plant, 0);
            const int heading = int(rnd() % 3u) - 1;
            const uint32_t jitter = rnd() % 36u;
            if (heading < 0) above.shade = uint8_t(jitter);
            else if (heading > 0) above.shade = uint8_t(114 + jitter);
            else above.shade = uint8_t(52 + jitter);
        }
    }

    if (self.id == Material::Plant) {
        if (smolder(x, y)) return true;
        const bool leaf = self.shade >= 200;
        const bool tip = y == 0 ||
            (peek(x, y - 1).id != Material::Plant &&
             peek(x, y - 1).id != Material::Wood);
        if (!leaf && self.life == 0 && tip && rooted(x, y) && (rnd() % 10u) == 0) growTree(x, y);
        if (!leaf && self.life == 0 && y > 0) {
            const Material above = peek(x, y - 1).id;
            if ((above == Material::Plant || above == Material::Wood) && (rnd() % 18u) == 0) {
                become(self, x, y, Material::Wood, 0);
            }
        }
    }

    if (self.id == Material::Ember && self.life <= 22) {
        constexpr int kLandingOrder[] = {2, 0, 1, 3};
        for (int direction : kLandingOrder) {
            int nx = 0;
            int ny = 0;
            if (!neighborAt(x, y, direction, nx, ny)) continue;
            const Material neighbor = peek(nx, ny).id;
            if (neighbor != Material::Gunpowder && neighbor != Material::Oil) continue;
            Cell& other = at(nx, ny);
            if (other.id == Material::Gunpowder) {
                explode(nx, ny);
                return true;
            }
            if (other.id == Material::Oil) {
                become(other, nx, ny, Material::Fire, uint8_t(70 + (rnd() % 30u)));
                spawn(nx, ny - 1, Material::Smoke, uint8_t(46 + (rnd() % 30u)));
                break;
            }
        }
    }

    if (self.id == Material::Fire || self.id == Material::Lava) {
        const int heat = self.id == Material::Lava ? 4 : 2;
        for (int i = 0; i < 4; ++i) {
            int nx = 0;
            int ny = 0;
            if (!neighborAt(x, y, i, nx, ny)) continue;
            const Material neighbor = peek(nx, ny).id;
            if (neighbor != Material::Gunpowder && neighbor != Material::Oil) continue;
            Cell& other = at(nx, ny);
            if (other.id == Material::Gunpowder) {
                explode(nx, ny);
                return true;
            }
            if (other.id == Material::Oil) {
                other.life = uint8_t(std::min(255, int(other.life) + heat * 6));
                if (other.life >= 48) {
                    become(other, nx, ny, Material::Fire, uint8_t(70 + (rnd() % 30u)));
                    spawn(nx, ny - 1, Material::Smoke, uint8_t(70 + (rnd() % 40u)));
                    spawn(nx, ny - 2, Material::Ember, uint8_t(18 + (rnd() % 14u)));
                }
            }
        }
        if (self.id == Material::Lava) radiate(x, y, 6, 6);
        else radiate(x, y, 3, 4);
    }

    if (self.id == Material::Glass) {
        for (int i = 0; i < 4; ++i) {
            int nx = 0;
            int ny = 0;
            if (!neighborAt(x, y, i, nx, ny)) continue;
            const Cell& water = peek(nx, ny);
            if (water.id != Material::Water || water.life != 0) continue;
            quench(x, y);
            return true;
        }
    }

    return false;
}

bool ParticleSimulator::trySwap(int x, int y, int nx, int ny) {
    if (!inBounds(nx, ny) || (x == nx && y == ny)) return false;
    const Cell& dstPeek = peek(nx, ny);
    if (dstPeek.id != Material::Empty && dstPeek.updated) return false;
    const Material srcId = peek(x, y).id;
    const Material dstId = dstPeek.id;
    const bool canMove = dstId == Material::Empty ||
        (infoOf(dstId).phase != Phase::Static && infoOf(srcId).density > infoOf(dstId).density);
    if (!canMove) return false;
    moveCell(x, y, nx, ny);
    return true;
}

void ParticleSimulator::moveCell(int x, int y, int nx, int ny) {
    const Material srcId = peek(x, y).id;
    const Material dstId = peek(nx, ny).id;
    Cell& src = at(x, y);
    Cell& dst = at(nx, ny);
    src.updated = 1;
    dst.updated = 1;
    std::swap(src, dst);
    account(x, y, srcId, dstId);
    account(nx, ny, dstId, srcId);
}

bool ParticleSimulator::drop(int x, int y, int maxDistance) {
    int landed = y;
    for (int step = 1; step <= maxDistance; ++step) {
        const int ny = y + step;
        if (!inBounds(x, ny) || peek(x, ny).id != Material::Empty) break;
        landed = ny;
    }
    return landed != y && trySwap(x, y, x, landed);
}

bool ParticleSimulator::slide(int x, int y, int dx, int maxDistance) {
    int landed = x;
    for (int step = 1; step <= maxDistance; ++step) {
        const int nx = x + dx * step;
        if (!inBounds(nx, y) || peek(nx, y).id != Material::Empty) break;
        landed = nx;
    }
    return landed != x && trySwap(x, y, landed, y);
}

void ParticleSimulator::spawn(int x, int y, Material material, uint8_t life) {
    if (!inBounds(x, y) || peek(x, y).id != Material::Empty) return;
    Cell& cell = at(x, y);
    become(cell, x, y, material, life);
}

void ParticleSimulator::moveFire(int x, int y) {
    const bool feeding = peek(x, y).life > 100;
    if ((rnd() % 20u) == 0) {
        const int spark = (rnd() & 1u) ? 1 : -1;
        spawn(x + spark, y - 1, Material::Ember, uint8_t(16 + (rnd() % 18u)));
    }
    if (feeding) {
        if ((rnd() & 3u) == 0) {
            const int direction = (rnd() & 1u) ? 1 : -1;
            trySwap(x, y, x + direction, y);
        }
        return;
    }
    if ((rnd() % 3u) != 0 && inBounds(x, y - 2) &&
        peek(x, y - 1).id == Material::Empty &&
        peek(x, y - 2).id == Material::Empty &&
        trySwap(x, y, x, y - 2)) {
        return;
    }
    if (trySwap(x, y, x, y - 1)) return;
    const int direction = (rnd() & 1u) ? 1 : -1;
    if (trySwap(x, y, x + direction, y - 1)) return;
    if (trySwap(x, y, x - direction, y - 1)) return;
    if ((rnd() & 3u) == 0) trySwap(x, y, x + direction, y);
}

void ParticleSimulator::moveEmber(int x, int y) {
    const bool hot = peek(x, y).life > 22;
    const int direction = (rnd() & 1u) ? 1 : -1;
    if (hot) {
        if (trySwap(x, y, x + direction, y - 1)) return;
        if (trySwap(x, y, x, y - 1)) return;
    }
    if ((m_frame & 1u) != 0) return;
    if (drop(x, y, 2)) return;
    trySwap(x, y, x + direction, y + 1);
}

void ParticleSimulator::moveSmoke(int x, int y) {
    const int direction = (rnd() & 1u) ? 1 : -1;
    if ((rnd() % 3u) != 0 && trySwap(x, y, x + direction, y)) return;
    if ((m_frame & 1u) == 0 && trySwap(x, y, x, y - 1)) return;
    if (trySwap(x, y, x + direction, y - 1)) return;
    trySwap(x, y, x - direction, y - 1);
}

void ParticleSimulator::moveSteam(int x, int y) {
    const int direction = (rnd() & 1u) ? 1 : -1;
    if (peek(x, y).life <= 24) {
        if (drop(x, y, 2)) return;
        if (trySwap(x, y, x + direction, y + 1)) return;
        if (trySwap(x, y, x - direction, y + 1)) return;
        trySwap(x, y, x + direction, y);
        return;
    }
    if (inBounds(x, y - 2) &&
        peek(x, y - 1).id == Material::Empty &&
        peek(x, y - 2).id == Material::Empty &&
        trySwap(x, y, x, y - 2)) {
        return;
    }
    if (trySwap(x, y, x + direction, y - 1)) return;
    if (trySwap(x, y, x - direction, y - 1)) return;
    if (trySwap(x, y, x, y - 1)) return;
    trySwap(x, y, x + direction, y);
}

void ParticleSimulator::movePowder(int x, int y) {
    if (drop(x, y, 3)) return;
    if (trySwap(x, y, x, y + 1)) return;
    const int direction = (rnd() & 1u) ? 1 : -1;
    if (trySwap(x, y, x + direction, y + 1)) return;
    trySwap(x, y, x - direction, y + 1);
}

void ParticleSimulator::moveLava(int x, int y) {
    constexpr int kFall = 5;
    int landedY = y;
    bool hitLighter = false;
    for (int step = 1; step <= kFall; ++step) {
        const int ny = y + step;
        if (!inBounds(x, ny)) break;
        const Cell& below = peek(x, ny);
        if (below.updated && below.id != Material::Empty) break;
        if (below.id == Material::Empty) {
            landedY = ny;
            continue;
        }
        const MaterialInfo& other = infoOf(below.id);
        const bool heavier = other.phase != Phase::Static && infoOf(Material::Lava).density > other.density;
        if (!heavier || hitLighter) break;
        landedY = ny;
        hitLighter = true;
    }
    if (landedY != y && trySwap(x, y, x, landedY)) return;

    const int direction = (rnd() & 1u) ? 1 : -1;
    if (trySwap(x, y, x + direction, y + 1)) return;
    trySwap(x, y, x - direction, y + 1);
}

void ParticleSimulator::moveLiquid(int x, int y) {
    if (drop(x, y, 3)) return;
    if (trySwap(x, y, x, y + 1)) return;
    const int direction = (rnd() & 1u) ? 1 : -1;
    if (trySwap(x, y, x + direction, y + 1)) return;
    if (trySwap(x, y, x - direction, y + 1)) return;
    if (slide(x, y, direction, 2)) return;
    slide(x, y, -direction, 2);
}

void ParticleSimulator::moveGas(int x, int y) {
    if (trySwap(x, y, x, y - 1)) return;
    const int direction = (rnd() & 1u) ? 1 : -1;
    if (trySwap(x, y, x + direction, y - 1)) return;
    if (trySwap(x, y, x - direction, y - 1)) return;
    if (trySwap(x, y, x + direction, y)) return;
    trySwap(x, y, x - direction, y);
}

void ParticleSimulator::shove(int cx, int cy, int radius, int lift, int power, bool liquidsOnly) {
    if (radius < 1 || power < 1) return;
    for (int ring = radius; ring >= 1; --ring) {
        const int kick = power + (radius - ring) * 2;
        for (int y = cy - ring; y <= cy + ring; ++y) {
            for (int x = cx - ring; x <= cx + ring; ++x) {
                if (std::max(std::abs(x - cx), std::abs(y - cy)) != ring) continue;
                if (!inBounds(x, y)) continue;
                if (!flungByBlast(peek(x, y).id, liquidsOnly)) continue;
                int px = x;
                int py = y;
                nudge(px, py, cx, cy, lift, kick);
            }
        }
    }
}

bool ParticleSimulator::nudge(int& x, int& y, int cx, int cy, int lift, int distance) {
    if (distance < 1) return false;
    const int dx = x - cx;
    const int rise = (y - cy) - lift;
    const int sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
    const int sy = rise < 0 || (rise == 0 && lift > 0) ? -1 : (rise > 0 ? 1 : 0);
    if (sx == 0 && sy == 0) return false;

    int px = x;
    int py = y;
    int landedX = x;
    int landedY = y;
    for (int step = 0; step < distance; ++step) {
        const int nx = px + sx;
        const int ny = py + sy;
        if (!inBounds(nx, ny) || peek(nx, ny).id != Material::Empty) break;
        landedX = nx;
        landedY = ny;
        px = nx;
        py = ny;
    }
    if (landedX == x && landedY == y) return false;
    moveCell(x, y, landedX, landedY);
    x = landedX;
    y = landedY;
    return true;
}

int ParticleSimulator::nearestHeat(int x, int y) const {
    int closest = 7;
    for (int dy = -6; dy <= 6; ++dy) {
        for (int dx = -6; dx <= 6; ++dx) {
            const int dist = std::max(std::abs(dx), std::abs(dy));
            if (dist == 0 || dist >= closest) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!inBounds(nx, ny)) continue;
            const Cell& other = at(nx, ny);
            int reach = 0;
            if (other.id == Material::Lava) reach = 6;
            else if (other.id == Material::Fire) reach = 4;
            else if (other.id == Material::Stone && other.life > 0) {
                if (other.life >= 120) reach = 5;
                else if (other.life >= 60) reach = 3;
                else reach = 1;
            }
            if (dist <= reach) closest = dist;
        }
    }
    return closest;
}

int ParticleSimulator::smolderDelay(int x, int y) const {
    for (int i = 0; i < 4; ++i) {
        int nx = 0;
        int ny = 0;
        if (!neighborAt(x, y, i, nx, ny)) continue;
        const Cell& other = at(nx, ny);
        if (other.id == Material::Lava) return 1;
        if (other.id == Material::Fire) return 1;
        if (other.id == Material::Ember) return 2;
    }

    int closest = 7;
    for (int dy = -6; dy <= 6; ++dy) {
        for (int dx = -6; dx <= 6; ++dx) {
            const int dist = std::max(std::abs(dx), std::abs(dy));
            if (dist == 0 || dist >= closest) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!inBounds(nx, ny)) continue;
            const Cell& other = at(nx, ny);
            int reach = 0;
            if (other.id == Material::Lava) reach = 6;
            else if (other.id == Material::Fire) reach = 4;
            else if (other.id == Material::Ember) reach = 2;
            else if (other.id == Material::Stone && other.life >= 60) reach = 4;
            if (dist <= reach) closest = dist;
        }
    }
    if (closest > 6) return 0;
    return 2 + closest;
}

bool ParticleSimulator::smolder(int x, int y) {
    Cell& self = at(x, y);
    const bool plant = self.id == Material::Plant;
    const int ignite = plant ? 54 : 36;
    const int delay = smolderDelay(x, y);
    if (delay <= 0) {
        if (self.life > 0 && (m_frame & 7u) == 0) --self.life;
        return false;
    }

    const int period = std::max(1, plant && delay > 1 ? delay + 1 : delay);
    if ((m_frame % uint32_t(period)) != 0) return false;
    if (self.life < ignite) ++self.life;
    if (self.life * 2 >= ignite && self.life < ignite && (rnd() % 4u) == 0) {
        spawn(x, y - 1, Material::Smoke, uint8_t(28 + (rnd() % 20u)));
    }
    if (self.life >= ignite) {
        become(self, x, y, Material::Fire, uint8_t(170 + (rnd() % 40u)));
        spawn(x, y - 1, Material::Smoke, uint8_t(40 + (rnd() % 30u)));
        spawn(x + ((rnd() & 1u) ? 1 : -1), y - 1, Material::Ember, uint8_t(16 + (rnd() % 12u)));
        return true;
    }
    return false;
}

void ParticleSimulator::radiate(int x, int y, int power, int reach) {
    if (power < 1 || reach < 1) return;
    constexpr int kBake = 120;
    for (int dy = -reach; dy <= reach; ++dy) {
        for (int dx = -reach; dx <= reach; ++dx) {
            const int dist = std::max(std::abs(dx), std::abs(dy));
            if (dist == 0 || dist > reach) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!inBounds(nx, ny)) continue;
            const Cell& probed = peek(nx, ny);
            if (probed.id != Material::Sand && probed.id != Material::Oil && probed.id != Material::Gunpowder) {
                continue;
            }
            Cell& other = at(nx, ny);
            const int gain = std::max(1, power - dist + 1);
            if (other.id == Material::Sand) {
                other.life = uint8_t(std::min(255, int(other.life) + gain));
                if (other.life >= kBake) become(other, nx, ny, Material::Glass, 0);
            } else if (other.id == Material::Oil && dist > 1) {
                other.life = uint8_t(std::min(255, int(other.life) + gain));
                if (other.life >= 48) {
                    become(other, nx, ny, Material::Fire, uint8_t(70 + (rnd() % 30u)));
                    spawn(nx, ny - 1, Material::Smoke, uint8_t(50 + (rnd() % 30u)));
                }
            } else if (other.id == Material::Gunpowder) {
                other.life = uint8_t(std::min(255, int(other.life) + std::max(1, gain / 2)));
                if (other.life >= 80) {
                    explode(nx, ny);
                    return;
                }
            }
        }
    }
}

void ParticleSimulator::quench(int x, int y) {
    constexpr int kCrack = 2;
    for (int dy = -kCrack; dy <= kCrack; ++dy) {
        for (int dx = -kCrack; dx <= kCrack; ++dx) {
            if (std::max(std::abs(dx), std::abs(dy)) > kCrack) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!inBounds(nx, ny)) continue;
            if (peek(nx, ny).id != Material::Glass) continue;
            Cell& cell = at(nx, ny);
            become(cell, nx, ny, Material::Sand, 0);
        }
    }
    shove(x, y, 5, 4, 5, false);
}

void ParticleSimulator::explode(int cx, int cy) {
    constexpr int kRadius = 4;
    constexpr int kBreak = 6;
    for (int y = cy - kBreak; y <= cy + kBreak; ++y) {
        for (int x = cx - kBreak; x <= cx + kBreak; ++x) {
            if (!inBounds(x, y)) continue;
            if (std::max(std::abs(x - cx), std::abs(y - cy)) > kBreak) continue;
            if (peek(x, y).id != Material::Glass) continue;
            Cell& cell = at(x, y);
            become(cell, x, y, Material::Sand, 0);
        }
    }
    shove(cx, cy, 16, 9, 12, false);
    for (int y = cy - kRadius; y <= cy + kRadius; ++y) {
        for (int x = cx - kRadius; x <= cx + kRadius; ++x) {
            if (!inBounds(x, y)) continue;
            const int dx = x - cx;
            const int dy = y - cy;
            const int dist2 = dx * dx + dy * dy;
            if (dist2 > kRadius * kRadius) continue;
            Cell& cell = at(x, y);
            if (cell.id == Material::Stone || cell.id == Material::Lava) continue;
            if (dist2 <= 2) {
                become(cell, x, y, Material::Empty, 0);
            } else if (dist2 <= 8) {
                become(cell, x, y, Material::Fire, uint8_t(150 + (rnd() % 20u)));
            } else if (dy < 0 && cell.id == Material::Empty) {
                become(cell, x, y, Material::Smoke, uint8_t(60 + (rnd() % 40u)));
            } else {
                become(cell, x, y, Material::Ember, uint8_t(20 + (rnd() % 24u)));
            }
        }
    }
}

bool ParticleSimulator::rooted(int x, int y) const {
    auto holds = [](const Cell& cell, bool lateral) {
        switch (cell.id) {
        case Material::Wood:
            return true;
        case Material::Plant:
            return cell.shade < 200;
        case Material::Stone:
        case Material::Glass:
        case Material::Sand:
        case Material::Salt:
        case Material::Water:
            return !lateral;
        case Material::Empty:
        case Material::Fire:
        case Material::Smoke:
        case Material::Ember:
        case Material::Steam:
        case Material::Gunpowder:
        case Material::Oil:
        case Material::Lava:
        case Material::Acid:
        case Material::Count:
            return false;
        }
        const Material unhandled = cell.id;
        throw std::logic_error("unhandled material " + std::to_string(int(unhandled)));
    };

    for (int dx = -1; dx <= 1; ++dx) {
        const int nx = x + dx;
        const int ny = y + 1;
        if (!inBounds(nx, ny)) continue;
        if (holds(at(nx, ny), false)) return true;
    }
    for (int dx = -1; dx <= 1; dx += 2) {
        if (!inBounds(x + dx, y)) continue;
        if (holds(at(x + dx, y), true)) return true;
    }
    return false;
}

bool ParticleSimulator::drinkFromRoot(int x, int y) {
    int cx = x;
    int cy = y;
    for (int step = 0; step < 64; ++step) {
        int nextX = cx;
        int nextY = cy;
        bool found = false;
        if (inBounds(cx, cy + 1)) {
            const Material below = peek(cx, cy + 1).id;
            if (below == Material::Plant || below == Material::Wood) {
                nextX = cx;
                nextY = cy + 1;
                found = true;
            }
        }
        if (!found) {
            for (int dx = -1; dx <= 1 && !found; dx += 2) {
                if (!inBounds(cx + dx, cy + 1)) continue;
                const Material below = peek(cx + dx, cy + 1).id;
                if (below != Material::Plant && below != Material::Wood) continue;
                nextX = cx + dx;
                nextY = cy + 1;
                found = true;
            }
        }
        if (!found) break;
        cx = nextX;
        cy = nextY;
    }

    constexpr int kRoot = 5;
    for (int radius = 1; radius <= kRoot; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != radius) continue;
                const int nx = cx + dx;
                const int ny = cy + dy;
                if (!inBounds(nx, ny)) continue;
                const Cell& waterPeek = peek(nx, ny);
                if (waterPeek.id != Material::Water || waterPeek.life != 0) continue;
                Cell& water = at(nx, ny);
                become(water, nx, ny, Material::Empty, 0);
                return true;
            }
        }
    }
    return false;
}

bool ParticleSimulator::growTree(int x, int y) {
    const Cell& self = peek(x, y);
    if (self.shade >= 200) return false;

    int heading = self.shade < 50 ? -1 : (self.shade >= 110 ? 1 : 0);
    const uint32_t drift = rnd() % 6u;
    if (drift == 0) heading = std::max(-1, heading - 1);
    else if (drift == 1) heading = std::min(1, heading + 1);

    int shootX = x + heading;
    int shootY = y - 1;
    int shootHeading = heading;
    if (!inBounds(shootX, shootY) || peek(shootX, shootY).id != Material::Empty) {
        shootX = x;
        shootY = y - 1;
        shootHeading = 0;
    }
    if (!inBounds(shootX, shootY) || peek(shootX, shootY).id != Material::Empty) {
        const int side = heading != 0 ? heading : ((rnd() & 1u) ? 1 : -1);
        shootX = x + side;
        shootY = y;
        shootHeading = side;
    }
    if (!inBounds(shootX, shootY) || peek(shootX, shootY).id != Material::Empty) return false;
    if (!drinkFromRoot(x, y)) return false;

    Cell& shoot = at(shootX, shootY);
    become(shoot, shootX, shootY, Material::Plant, 0);
    const uint32_t jitter = rnd() % 36u;
    if (shootHeading < 0) shoot.shade = uint8_t(jitter);
    else if (shootHeading > 0) shoot.shade = uint8_t(114 + jitter);
    else shoot.shade = uint8_t(52 + jitter);

    if ((rnd() % 2u) == 0) {
        const int leafSide = shootHeading != 0 ? shootHeading : ((rnd() & 1u) ? 1 : -1);
        const int leafX = x + leafSide;
        const int leafY = (rnd() & 1u) ? y - 1 : y;
        if (inBounds(leafX, leafY) && peek(leafX, leafY).id == Material::Empty) {
            Cell& leaf = at(leafX, leafY);
            become(leaf, leafX, leafY, Material::Plant, 0);
            leaf.shade = uint8_t(214 + (rnd() % 36u));
        }
    }

    if ((rnd() % 4u) == 0) {
        const int fork = shootHeading == 0 ? ((rnd() & 1u) ? 1 : -1) : -shootHeading;
        const int forkX = x + fork;
        const int forkY = y - 1;
        const bool forkUp = inBounds(forkX, forkY) && peek(forkX, forkY).id == Material::Empty;
        const int branchX = forkUp ? forkX : x + fork;
        const int branchY = forkUp ? forkY : y;
        if (inBounds(branchX, branchY) && peek(branchX, branchY).id == Material::Empty &&
            drinkFromRoot(x, y)) {
            Cell& branch = at(branchX, branchY);
            become(branch, branchX, branchY, Material::Plant, 0);
            const uint32_t branchJitter = rnd() % 36u;
            branch.shade = fork < 0 ? uint8_t(branchJitter) : uint8_t(114 + branchJitter);
        }
    }
    return true;
}

void ParticleSimulator::become(Cell& cell, int x, int y, Material material, uint8_t life) {
    if (&cell == &m_sink || !inBounds(x, y)) return;
    const Material old = cell.id;
    cell.id = material;
    cell.life = life;
    cell.shade = uint8_t(rnd() & 255u);
    cell.updated = 1;
    account(x, y, old, material);
}

bool ParticleSimulator::neighborAt(int x, int y, int direction, int& nx, int& ny) const {
    static constexpr int kDx[4] = {1, -1, 0, 0};
    static constexpr int kDy[4] = {0, 0, 1, -1};
    nx = x + kDx[direction];
    ny = y + kDy[direction];
    return inBounds(nx, ny);
}

bool ParticleSimulator::inBounds(int x, int y) const {
    constexpr int kLimit = 10000000;
    return x > -kLimit && x < kLimit && y > -kLimit && y < kLimit;
}

ParticleSimulator::ChunkKey ParticleSimulator::chunkKey(int x, int y) const {
    const auto axis = [](int value) {
        if (value >= 0) return value / kChunkSize;
        return -((-value + kChunkSize - 1) / kChunkSize);
    };
    return {axis(x), axis(y)};
}

int ParticleSimulator::chunkLocal(int x, int y) const {
    const ChunkKey key = chunkKey(x, y);
    const int localX = x - key.x * kChunkSize;
    const int localY = y - key.y * kChunkSize;
    return localY * kChunkSize + localX;
}

ParticleSimulator::Cell& ParticleSimulator::at(int x, int y) {
    if (!inBounds(x, y)) {
        m_sink = {};
        return m_sink;
    }
    const ChunkKey key = chunkKey(x, y);
    std::unique_ptr<Chunk>& slot = m_chunks[key];
    if (!slot) slot = std::make_unique<Chunk>();
    return slot->cells[chunkLocal(x, y)];
}

const ParticleSimulator::Cell& ParticleSimulator::peek(int x, int y) const {
    static const Cell kEmpty{};
    if (!inBounds(x, y)) return kEmpty;
    const Chunk* chunk = findChunk(x, y);
    if (chunk == nullptr) return kEmpty;
    return chunk->cells[chunkLocal(x, y)];
}

const ParticleSimulator::Cell& ParticleSimulator::at(int x, int y) const {
    return peek(x, y);
}

ParticleSimulator::Chunk* ParticleSimulator::findChunk(int x, int y) {
    if (!inBounds(x, y)) return nullptr;
    const auto found = m_chunks.find(chunkKey(x, y));
    if (found == m_chunks.end() || !found->second) return nullptr;
    return found->second.get();
}

const ParticleSimulator::Chunk* ParticleSimulator::findChunk(int x, int y) const {
    if (!inBounds(x, y)) return nullptr;
    const auto found = m_chunks.find(chunkKey(x, y));
    if (found == m_chunks.end() || !found->second) return nullptr;
    return found->second.get();
}

bool ParticleSimulator::keepsAwake(const Cell& cell) const {
    switch (cell.id) {
    case Material::Empty:
    case Material::Glass:
        return false;
    case Material::Stone:
    case Material::Sand:
    case Material::Salt:
    case Material::Gunpowder:
    case Material::Water:
    case Material::Oil:
        return cell.life > 0;
    case Material::Wood:
    case Material::Plant:
    case Material::Acid:
    case Material::Lava:
    case Material::Fire:
    case Material::Smoke:
    case Material::Ember:
    case Material::Steam:
        return true;
    case Material::Count:
        return false;
    }
    const Material unhandled = cell.id;
    throw std::logic_error("unhandled material " + std::to_string(int(unhandled)));
}

void ParticleSimulator::account(int x, int y, Material oldId, Material newId) {
    touch(x, y);
    if (oldId == newId) return;
    Chunk* chunk = findChunk(x, y);
    if (chunk == nullptr) return;

    const auto present = [](Material id) {
        return id != Material::Empty && id != Material::Count;
    };
    const auto anchor = [&present](Material id) {
        return present(id) && infoOf(id).phase == Phase::Static;
    };
    if (present(oldId) != present(newId)) {
        if (present(newId)) ++chunk->occupied;
        else if (chunk->occupied > 0) --chunk->occupied;
    }
    if (anchor(oldId) != anchor(newId)) {
        if (anchor(newId)) ++chunk->anchors;
        else if (chunk->anchors > 0) --chunk->anchors;
    }
}

void ParticleSimulator::touch(int x, int y) {
    Chunk* chunk = findChunk(x, y);
    if (chunk == nullptr) return;
    chunk->dirty = true;
    chunk->awake = true;

    const ChunkKey key = chunkKey(x, y);
    const int localX = x - key.x * kChunkSize;
    const int localY = y - key.y * kChunkSize;
    const int dx0 = localX == 0 ? -1 : 0;
    const int dx1 = localX == kChunkSize - 1 ? 1 : 0;
    const int dy0 = localY == 0 ? -1 : 0;
    const int dy1 = localY == kChunkSize - 1 ? 1 : 0;
    if (dx0 == 0 && dx1 == 0 && dy0 == 0 && dy1 == 0) return;
    for (int dy = dy0; dy <= dy1; ++dy) {
        for (int dx = dx0; dx <= dx1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const auto found = m_chunks.find(ChunkKey{key.x + dx, key.y + dy});
            if (found == m_chunks.end() || !found->second) continue;
            found->second->awake = true;
            found->second->woken = true;
        }
    }
}

void ParticleSimulator::wakeAround(ChunkKey key) {
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const auto found = m_chunks.find(ChunkKey{key.x + dx, key.y + dy});
            if (found == m_chunks.end() || !found->second) continue;
            found->second->awake = true;
            found->second->woken = true;
        }
    }
}

bool ParticleSimulator::hasSupport(ChunkKey key) const {
    constexpr int kDepth = 8;
    for (int dy = 1; dy <= kDepth; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const auto found = m_chunks.find(ChunkKey{key.x + dx, key.y + dy});
            if (found == m_chunks.end() || !found->second) continue;
            const Chunk& below = *found->second;
            if (below.anchors > 0 || (!below.awake && below.occupied > 0)) return true;
        }
    }
    return false;
}

void ParticleSimulator::cullBelow(int maxChunkY) {
    std::vector<ChunkKey> doomed;
    for (const auto& entry : m_chunks) {
        const Chunk& chunk = *entry.second;
        if (!chunk.awake || chunk.anchors > 0) continue;
        if (entry.first.y <= maxChunkY) continue;
        if (hasSupport(entry.first)) continue;
        doomed.push_back(entry.first);
    }
    for (const ChunkKey key : doomed) m_chunks.erase(key);
}

void ParticleSimulator::discardEmptyChunks() {
    for (auto it = m_chunks.begin(); it != m_chunks.end();) {
        if (it->second->occupied == 0) it = m_chunks.erase(it);
        else ++it;
    }
}

uint32_t ParticleSimulator::rnd() {
    uint32_t x = m_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m_rng = x;
    return x;
}
