#include "ParticleSim.h"

#include <algorithm>
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

bool dissolves(Material material) {
    switch (material) {
    case Material::Sand:
    case Material::Salt:
    case Material::Wood:
    case Material::Gunpowder:
    case Material::Oil:
    case Material::Ember:
        return true;
    case Material::Empty:
    case Material::Water:
    case Material::Fire:
    case Material::Smoke:
    case Material::Steam:
    case Material::Lava:
    case Material::Stone:
    case Material::Acid:
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
    m_cells.resize(cells);
    m_pixels.resize(cells * 4);
    seed();
}

void ParticleSimulator::setBrushRadius(int radius) {
    m_brush = std::clamp(radius, 1, 48);
}

void ParticleSimulator::update(float deltaTime) {
    if (!m_running) return;
    m_time += deltaTime;
    constexpr float kStep = 1.f / 60.f;
    if (m_time < kStep) return;
    step();
    m_time = std::min(m_time - kStep, kStep);
}

void ParticleSimulator::paint(int x, int y, int radius, Material material) {
    fill(x, y, radius, material);
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

void ParticleSimulator::clear() {
    m_cells.assign(m_cells.size(), {});
    publish();
}

void ParticleSimulator::seed() {
    for (int x = 0; x < m_width; ++x) {
        for (int y = m_height - 5; y < m_height; ++y) {
            setCell(x, y, Material::Stone);
        }
    }
    fill(m_width / 3, m_height / 3, 34, Material::Sand);
    fill(m_width / 2, m_height / 2 + 24, 30, Material::Water);
    fill(m_width * 2 / 3, m_height / 3, 16, Material::Oil);
    publish();
}

void ParticleSimulator::step() {
    ++m_frame;
    for (Cell& cell : m_cells) cell.updated = 0;

    const int direction = (m_frame & 1u) ? 1 : -1;
    const int xStart = direction > 0 ? 0 : m_width - 1;
    const int xEnd = direction > 0 ? m_width : -1;
    for (int y = m_height - 1; y >= 0; --y) {
        const size_t row = static_cast<size_t>(y) * static_cast<size_t>(m_width);
        for (int x = xStart; x != xEnd; x += direction) {
            if (m_cells[row + static_cast<size_t>(x)].id == Material::Empty) continue;
            updateCell(x, y);
        }
    }
}

void ParticleSimulator::publish() {
    uint8_t* out = m_pixels.data();
    for (const Cell& cell : m_cells) {
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
        } else if (cell.id == Material::Wood && cell.life > 0) {
            const int charred = std::min(int(cell.life) * 4, 180);
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
            const int cloud = std::min(int(cell.life), 80);
            r = r + (210 - r) * cloud / 120;
            g = g + (216 - g) * cloud / 120;
            b = b + (220 - b) * cloud / 120;
        } else if (cell.id == Material::Acid && cell.life > 0) {
            const int stain = std::min(int(cell.life), 40);
            r = r + (int(cell.shade) - r) * stain / 80;
            g = g + (80 - g) * stain / 160;
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
    Cell& cell = m_cells[index(x, y)];
    cell.id = material;
    cell.shade = uint8_t(rnd() & 255u);
    cell.updated = 0;
    cell.life = lifeFor(material, rnd());
}

void ParticleSimulator::updateCell(int x, int y) {
    const Cell& current = m_cells[index(x, y)];
    if (current.id == Material::Empty || current.updated) return;
    if (react(x, y)) return;

    const Cell& cell = m_cells[index(x, y)];
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
    Cell& self = m_cells[index(x, y)];
    if (self.id == Material::Water && self.life > 0 && (m_frame & 3u) == 0) {
        --self.life;
    }
    if (self.id == Material::Acid && self.life > 0 && (rnd() & 3u) == 0) {
        --self.life;
    }

    if (usesLife(self.id)) {
        if (self.life == 0) {
            if (self.id == Material::Fire) {
                become(self, Material::Smoke, uint8_t(50 + (rnd() % 50u)));
            } else if (self.id == Material::Ember && (rnd() & 1u) == 0) {
                become(self, Material::Smoke, uint8_t(30 + (rnd() % 30u)));
            } else {
                become(self, Material::Empty, 0);
            }
            return true;
        }
        --self.life;
        if (self.id == Material::Fire && self.life < 20 && (rnd() % 5u) == 0) {
            become(self, Material::Ember, uint8_t(24 + (rnd() % 20u)));
            return true;
        }
    }

    if (self.id == Material::Water) {
        for (int i = 0; i < 4; ++i) {
            int nx = 0;
            int ny = 0;
            if (!neighborAt(x, y, i, nx, ny)) continue;
            Cell& other = m_cells[index(nx, ny)];
            if (other.id == Material::Lava) {
                become(self, Material::Steam, uint8_t(100 + (rnd() % 40u)));
                spawn(x, y - 1, Material::Steam, uint8_t(80 + (rnd() % 30u)));
                spawn(x - 1, y - 1, Material::Steam, uint8_t(60 + (rnd() % 30u)));
                spawn(x + 1, y - 1, Material::Steam, uint8_t(60 + (rnd() % 30u)));
                spawn(x, y - 2, Material::Steam, uint8_t(40 + (rnd() % 20u)));
                become(other, Material::Stone, 0);
                spawn(nx - 1, ny, Material::Ember, uint8_t(20 + (rnd() % 16u)));
                spawn(nx + 1, ny, Material::Ember, uint8_t(20 + (rnd() % 16u)));
                return true;
            }
            if (other.id == Material::Fire) {
                become(self, Material::Steam, uint8_t(80 + (rnd() % 40u)));
                spawn(x, y - 1, Material::Steam, uint8_t(50 + (rnd() % 30u)));
                spawn(x + ((rnd() & 1u) ? 1 : -1), y - 1, Material::Steam, uint8_t(40 + (rnd() % 20u)));
                become(other, Material::Ember, uint8_t(28 + (rnd() % 16u)));
                spawn(nx, ny - 1, Material::Ember, uint8_t(18 + (rnd() % 12u)));
                return true;
            }
        }
    }

    if (self.id == Material::Salt) {
        for (int i = 0; i < 4; ++i) {
            int nx = 0;
            int ny = 0;
            if (!neighborAt(x, y, i, nx, ny)) continue;
            Cell& water = m_cells[index(nx, ny)];
            if (water.id != Material::Water || (rnd() % 10u) != 0) continue;
            self.life = uint8_t(std::min(255, int(self.life) + 1));
            water.life = uint8_t(std::min(80, int(water.life) + 28));
            if (self.life >= 5) {
                spawn(x, y - 1, Material::Steam, uint8_t(16 + (rnd() % 10u)));
                become(self, Material::Empty, 0);
                return true;
            }
        }
    }

    if (self.id == Material::Acid) {
        int nx = 0;
        int ny = 0;
        if (neighborAt(x, y, int(rnd() % 4u), nx, ny)) {
            Cell& other = m_cells[index(nx, ny)];
            if (dissolves(other.id) && (rnd() % 3u) == 0) {
                self.shade = infoOf(other.id).r;
                self.life = 40;
                become(other, Material::Steam, uint8_t(14 + (rnd() % 12u)));
                spawn(nx, ny - 1, Material::Steam, uint8_t(10 + (rnd() % 8u)));
                if ((rnd() % 6u) == 0) {
                    become(self, Material::Empty, 0);
                    return true;
                }
            }
        }
    }

    if (self.id == Material::Fire || self.id == Material::Lava) {
        const int heat = self.id == Material::Lava ? 4 : 2;
        for (int i = 0; i < 4; ++i) {
            int nx = 0;
            int ny = 0;
            if (!neighborAt(x, y, i, nx, ny)) continue;
            Cell& other = m_cells[index(nx, ny)];
            if (other.id == Material::Gunpowder) {
                explode(nx, ny);
                return true;
            }
            if (other.id == Material::Wood) {
                other.life = uint8_t(std::min(255, int(other.life) + heat));
                if (other.life >= 16 && (rnd() & 1u) == 0) {
                    become(other, Material::Fire, uint8_t(90 + (rnd() % 40u)));
                    spawn(nx, ny - 1, Material::Ember, uint8_t(22 + (rnd() % 16u)));
                    spawn(nx + ((rnd() & 1u) ? 1 : -1), ny - 1, Material::Smoke, uint8_t(40 + (rnd() % 30u)));
                }
            } else if (other.id == Material::Oil) {
                other.life = uint8_t(std::min(255, int(other.life) + heat * 6));
                if (other.life >= 48) {
                    become(other, Material::Fire, uint8_t(70 + (rnd() % 30u)));
                    spawn(nx, ny - 1, Material::Smoke, uint8_t(70 + (rnd() % 40u)));
                    spawn(nx, ny - 2, Material::Ember, uint8_t(18 + (rnd() % 14u)));
                }
            }
        }
    }

    return false;
}

bool ParticleSimulator::trySwap(int x, int y, int nx, int ny) {
    if (!inBounds(nx, ny) || (x == nx && y == ny)) return false;
    Cell& src = m_cells[index(x, y)];
    Cell& dst = m_cells[index(nx, ny)];
    if (dst.id != Material::Empty && dst.updated) return false;

    const bool canMove = dst.id == Material::Empty ||
        (infoOf(dst.id).phase != Phase::Static && infoOf(src.id).density > infoOf(dst.id).density);
    if (!canMove) return false;

    src.updated = 1;
    dst.updated = 1;
    std::swap(src, dst);
    return true;
}

bool ParticleSimulator::drop(int x, int y, int maxDistance) {
    int landed = y;
    for (int step = 1; step <= maxDistance; ++step) {
        const int ny = y + step;
        if (!inBounds(x, ny) || m_cells[index(x, ny)].id != Material::Empty) break;
        landed = ny;
    }
    return landed != y && trySwap(x, y, x, landed);
}

bool ParticleSimulator::slide(int x, int y, int dx, int maxDistance) {
    int landed = x;
    for (int step = 1; step <= maxDistance; ++step) {
        const int nx = x + dx * step;
        if (!inBounds(nx, y) || m_cells[index(nx, y)].id != Material::Empty) break;
        landed = nx;
    }
    return landed != x && trySwap(x, y, landed, y);
}

void ParticleSimulator::spawn(int x, int y, Material material, uint8_t life) {
    if (!inBounds(x, y)) return;
    Cell& cell = m_cells[index(x, y)];
    if (cell.id != Material::Empty) return;
    become(cell, material, life);
}

void ParticleSimulator::moveFire(int x, int y) {
    if ((rnd() % 20u) == 0) {
        const int spark = (rnd() & 1u) ? 1 : -1;
        spawn(x + spark, y - 1, Material::Ember, uint8_t(16 + (rnd() % 18u)));
    }
    if ((rnd() % 3u) != 0 && inBounds(x, y - 2) &&
        m_cells[index(x, y - 1)].id == Material::Empty &&
        m_cells[index(x, y - 2)].id == Material::Empty &&
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
    const bool hot = m_cells[index(x, y)].life > 22;
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
    if (inBounds(x, y - 2) &&
        m_cells[index(x, y - 1)].id == Material::Empty &&
        m_cells[index(x, y - 2)].id == Material::Empty &&
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
        const Cell& below = m_cells[index(x, ny)];
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
    if (drop(x, y, 2)) return;
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

void ParticleSimulator::explode(int cx, int cy) {
    constexpr int kRadius = 4;
    for (int y = cy - kRadius; y <= cy + kRadius; ++y) {
        for (int x = cx - kRadius; x <= cx + kRadius; ++x) {
            if (!inBounds(x, y)) continue;
            const int dx = x - cx;
            const int dy = y - cy;
            const int dist2 = dx * dx + dy * dy;
            if (dist2 > kRadius * kRadius) continue;
            Cell& cell = m_cells[index(x, y)];
            if (cell.id == Material::Stone || cell.id == Material::Lava) continue;
            if (dist2 <= 2) {
                become(cell, Material::Empty, 0);
            } else if (dist2 <= 8) {
                become(cell, Material::Fire, uint8_t(150 + (rnd() % 20u)));
            } else if (dy < 0 && cell.id == Material::Empty) {
                become(cell, Material::Smoke, uint8_t(60 + (rnd() % 40u)));
            } else {
                become(cell, Material::Ember, uint8_t(20 + (rnd() % 24u)));
            }
        }
    }
}

void ParticleSimulator::become(Cell& cell, Material material, uint8_t life) {
    cell.id = material;
    cell.life = life;
    cell.shade = uint8_t(rnd() & 255u);
    cell.updated = 1;
}

bool ParticleSimulator::neighborAt(int x, int y, int direction, int& nx, int& ny) const {
    static constexpr int kDx[4] = {1, -1, 0, 0};
    static constexpr int kDy[4] = {0, 0, 1, -1};
    nx = x + kDx[direction];
    ny = y + kDy[direction];
    return inBounds(nx, ny);
}

bool ParticleSimulator::inBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < m_width && y < m_height;
}

int ParticleSimulator::index(int x, int y) const {
    return y * m_width + x;
}

uint32_t ParticleSimulator::rnd() {
    uint32_t x = m_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m_rng = x;
    return x;
}
