#include "Hud.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

namespace {

constexpr int kPanelWidth = 118;
constexpr int kRowHeight = 18;
constexpr int kListTop = 8;

constexpr Material kPalette[] = {
    Material::Sand,
    Material::Water,
    Material::Salt,
    Material::Wood,
    Material::Plant,
    Material::Fire,
    Material::Smoke,
    Material::Ember,
    Material::Steam,
    Material::Gunpowder,
    Material::Oil,
    Material::Lava,
    Material::Stone,
    Material::Acid,
};

// 5x7 glyphs, bit 4 is the left pixel. Indexed as '0'-'9' then 'A'-'Z'.
constexpr uint8_t kGlyphs[][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // 2
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}, // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
    {0x0E, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x0E}, // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}, // 9
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // A
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // B
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, // C
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, // D
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // E
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // F
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, // G
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // H
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // I
    {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}, // J
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, // N
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // O
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // P
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, // Q
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // R
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // S
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // U
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, // V
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}, // W
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // X
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, // Y
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // Z
};

const uint8_t* glyphRows(char letter) {
    if (letter >= 'a' && letter <= 'z') letter = static_cast<char>(letter - 32);
    if (letter >= '0' && letter <= '9') return kGlyphs[letter - '0'];
    if (letter >= 'A' && letter <= 'Z') return kGlyphs[10 + letter - 'A'];
    return nullptr;
}

void fillRect(uint8_t* pixels, int width, int height, int x, int y, int w, int h,
              uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const int x1 = std::max(x, 0);
    const int y1 = std::max(y, 0);
    const int x2 = std::min(x + w, width);
    const int y2 = std::min(y + h, height);
    for (int py = y1; py < y2; ++py) {
        uint8_t* row = pixels + (static_cast<size_t>(py) * static_cast<size_t>(width) + static_cast<size_t>(x1)) * 4;
        for (int px = x1; px < x2; ++px) {
            row[0] = r;
            row[1] = g;
            row[2] = b;
            row[3] = a;
            row += 4;
        }
    }
}

void drawGlyph(uint8_t* pixels, int width, int height, int x, int y, char letter) {
    const uint8_t* rows = glyphRows(letter);
    if (!rows) return;
    for (int row = 0; row < 7; ++row) {
        const int py = y + row;
        if (py < 0 || py >= height) continue;
        for (int col = 0; col < 5; ++col) {
            if ((rows[row] & (0x10 >> col)) == 0) continue;
            const int px = x + col;
            if (px < 0 || px >= width) continue;
            uint8_t* pixel = pixels + (static_cast<size_t>(py) * static_cast<size_t>(width) + static_cast<size_t>(px)) * 4;
            pixel[0] = pixel[1] = pixel[2] = 235;
            pixel[3] = 255;
        }
    }
}

void drawText(uint8_t* pixels, int width, int height, int x, int y, const char* text) {
    for (int i = 0; text[i] != '\0'; ++i) {
        drawGlyph(pixels, width, height, x, y, text[i]);
        x += 6;
    }
}

}  // namespace

const uint8_t* hudGlyphRows(char letter) { return glyphRows(letter); }

int hudWidth() { return kPanelWidth; }

std::optional<Material> hudMaterialAt(int x, int y) {
    if (x < 0 || x >= kPanelWidth || y < kListTop) return std::nullopt;
    const int row = (y - kListTop) / kRowHeight;
    if (row < 0 || row >= static_cast<int>(std::size(kPalette))) return std::nullopt;
    if ((y - kListTop) % kRowHeight > kRowHeight - 2) return std::nullopt;
    return kPalette[row];
}

void drawHud(uint8_t* pixels, int width, int height, const ParticleSimulator& simulation, float fps) {
    if (width < kPanelWidth || height < 32) return;

    fillRect(pixels, width, height, 0, 0, kPanelWidth, height, 16, 16, 18, 255);

    for (int i = 0; i < static_cast<int>(std::size(kPalette)); ++i) {
        const int y = kListTop + i * kRowHeight;
        const Material material = kPalette[i];
        if (material == simulation.material()) {
            fillRect(pixels, width, height, 2, y - 2, kPanelWidth - 6, kRowHeight - 2, 48, 48, 54, 230);
        }
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        simulation.materialRgb(material, r, g, b);
        fillRect(pixels, width, height, 6, y, 10, 10, r, g, b, 255);
        drawText(pixels, width, height, 22, y + 1, materialName(material));
    }

    const int statusY = kListTop + static_cast<int>(std::size(kPalette)) * kRowHeight + 8;
    char line[32];
    std::snprintf(line, sizeof(line), "%.0f FPS", fps);
    drawText(pixels, width, height, 8, statusY, line);
    std::snprintf(line, sizeof(line), "R %d", simulation.brushRadius());
    drawText(pixels, width, height, 8, statusY + 12, line);
    if (!simulation.running()) drawText(pixels, width, height, 8, statusY + 24, "PAUSED");
}
