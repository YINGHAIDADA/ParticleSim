#include "Hud.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

constexpr int kPanelWidth = 118;
constexpr int kRowHeight = 18;
constexpr int kListTop = 8;
constexpr int kToolbarHeight = 22;
constexpr int kButtonHeight = 18;
constexpr int kButtonY = 2;

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

int toolbarHeight() { return kToolbarHeight; }

namespace {

struct BarButton {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    ToolbarHit hit = ToolbarHit::Brush;
    const char* label = "";
};

int placeButton(BarButton* buttons, int count, int x, ToolbarHit hit, const char* label) {
    const int width = int(std::strlen(label)) * 6 + 12;
    buttons[count] = BarButton{x, kButtonY, width, kButtonHeight, hit, label};
    return x + width + 4;
}

int toolbarButtons(BarButton* buttons) {
    int x = kPanelWidth + 6;
    int count = 0;
    x = placeButton(buttons, count++, x, ToolbarHit::Brush, "DRAW");
    x = placeButton(buttons, count++, x, ToolbarHit::Hand, "HAND");
    x = placeButton(buttons, count++, x, ToolbarHit::Shape, "SHAPE");
    x += 8;
    x = placeButton(buttons, count++, x, ToolbarHit::Rect, "RECT");
    x = placeButton(buttons, count++, x, ToolbarHit::Circle, "CIRCLE");
    x += 8;
    x = placeButton(buttons, count++, x, ToolbarHit::Solid, "FILL");
    placeButton(buttons, count++, x, ToolbarHit::Stroke, "STROKE");
    return count;
}

enum class ButtonTone : uint8_t { Idle, Armed, Active };

ButtonTone buttonTone(const BarButton& button, CanvasTool tool, ShapeForm form, ShapeStyle style) {
    switch (button.hit) {
    case ToolbarHit::Brush:
        return tool == CanvasTool::Brush ? ButtonTone::Active : ButtonTone::Idle;
    case ToolbarHit::Hand:
        return tool == CanvasTool::Hand ? ButtonTone::Active : ButtonTone::Idle;
    case ToolbarHit::Shape:
        return tool == CanvasTool::Shape ? ButtonTone::Active : ButtonTone::Idle;
    case ToolbarHit::Rect:
    case ToolbarHit::Circle: {
        const bool armed = (button.hit == ToolbarHit::Rect && form == ShapeForm::Rect) ||
            (button.hit == ToolbarHit::Circle && form == ShapeForm::Circle);
        if (!armed) return ButtonTone::Idle;
        return tool == CanvasTool::Shape ? ButtonTone::Active : ButtonTone::Armed;
    }
    case ToolbarHit::Solid:
    case ToolbarHit::Stroke: {
        const bool armed = (button.hit == ToolbarHit::Solid && style == ShapeStyle::Solid) ||
            (button.hit == ToolbarHit::Stroke && style == ShapeStyle::Stroke);
        if (!armed) return ButtonTone::Idle;
        return tool == CanvasTool::Shape ? ButtonTone::Active : ButtonTone::Armed;
    }
    }
    const ToolbarHit unhandled = button.hit;
    throw std::logic_error("unhandled toolbar hit " + std::to_string(int(unhandled)));
}

void toneColor(ButtonTone tone, uint8_t& r, uint8_t& g, uint8_t& b) {
    switch (tone) {
    case ButtonTone::Idle:
        r = 28;
        g = 28;
        b = 32;
        return;
    case ButtonTone::Armed:
        r = 36;
        g = 42;
        b = 52;
        return;
    case ButtonTone::Active:
        r = 42;
        g = 64;
        b = 96;
        return;
    }
    const ButtonTone unhandled = tone;
    throw std::logic_error("unhandled button tone " + std::to_string(int(unhandled)));
}

}  // namespace

std::optional<Material> hudMaterialAt(int x, int y) {
    if (x < 0 || x >= kPanelWidth || y < kListTop) return std::nullopt;
    const int row = (y - kListTop) / kRowHeight;
    if (row < 0 || row >= static_cast<int>(std::size(kPalette))) return std::nullopt;
    if ((y - kListTop) % kRowHeight > kRowHeight - 2) return std::nullopt;
    return kPalette[row];
}

std::optional<ToolbarHit> toolbarAt(int x, int y) {
    if (x < kPanelWidth || y < 0 || y >= kToolbarHeight) return std::nullopt;
    BarButton buttons[8];
    const int count = toolbarButtons(buttons);
    for (int i = 0; i < count; ++i) {
        const BarButton& button = buttons[i];
        if (x >= button.x && x < button.x + button.w && y >= button.y && y < button.y + button.h) {
            return button.hit;
        }
    }
    return std::nullopt;
}

bool shapeContains(int x, int y, int x0, int y0, int x1, int y1, ShapeForm form, ShapeStyle style, int stroke) {
    const int width = std::max(stroke, 1);
    switch (form) {
    case ShapeForm::Rect: {
        const int minX = std::min(x0, x1);
        const int maxX = std::max(x0, x1);
        const int minY = std::min(y0, y1);
        const int maxY = std::max(y0, y1);
        if (x < minX || x > maxX || y < minY || y > maxY) return false;
        switch (style) {
        case ShapeStyle::Solid:
            return true;
        case ShapeStyle::Stroke:
            return x < minX + width || x > maxX - width || y < minY + width || y > maxY - width;
        }
        const ShapeStyle unhandled = style;
        throw std::logic_error("unhandled shape style " + std::to_string(int(unhandled)));
    }
    case ShapeForm::Circle: {
        const int dx = x - x0;
        const int dy = y - y0;
        const int rdx = x1 - x0;
        const int rdy = y1 - y0;
        const int radiusSq = rdx * rdx + rdy * rdy;
        const int distSq = dx * dx + dy * dy;
        if (distSq > radiusSq) return false;
        const int inner = int(std::lround(std::sqrt(double(radiusSq)))) - width;
        switch (style) {
        case ShapeStyle::Solid:
            return true;
        case ShapeStyle::Stroke:
            if (inner <= 0) return true;
            return distSq >= inner * inner;
        }
        const ShapeStyle unhandled = style;
        throw std::logic_error("unhandled shape style " + std::to_string(int(unhandled)));
    }
    }
    const ShapeForm unhandled = form;
    throw std::logic_error("unhandled shape form " + std::to_string(int(unhandled)));
}

void drawShapePreview(
    uint8_t* pixels,
    int width,
    int height,
    float centerX,
    float centerY,
    float zoom,
    int x0,
    int y0,
    int x1,
    int y1,
    ShapeForm form,
    ShapeStyle style,
    int stroke,
    uint8_t red,
    uint8_t green,
    uint8_t blue
) {
    const int hud = hudWidth();
    const int bar = toolbarHeight();
    const float canvasW = float(std::max(width - hud, 1));
    const float canvasH = float(std::max(height - bar, 1));
    const float viewW = canvasW / std::max(zoom, 0.05f);
    const float viewH = canvasH / std::max(zoom, 0.05f);
    constexpr int kMix = 150;
    for (int py = bar; py < height; ++py) {
        for (int px = hud; px < width; ++px) {
            const float localX = (float(px - hud) + 0.5f) / canvasW;
            const float localY = (float(py - bar) + 0.5f) / canvasH;
            const int wx = int(std::floor(double(centerX) + (double(localX) - 0.5) * double(viewW)));
            const int wy = int(std::floor(double(centerY) + (double(localY) - 0.5) * double(viewH)));
            if (!shapeContains(wx, wy, x0, y0, x1, y1, form, style, stroke)) continue;
            uint8_t* pixel = pixels + (static_cast<size_t>(py) * static_cast<size_t>(width) + static_cast<size_t>(px)) * 4;
            pixel[0] = uint8_t((int(pixel[0]) * (255 - kMix) + int(red) * kMix) / 255);
            pixel[1] = uint8_t((int(pixel[1]) * (255 - kMix) + int(green) * kMix) / 255);
            pixel[2] = uint8_t((int(pixel[2]) * (255 - kMix) + int(blue) * kMix) / 255);
            pixel[3] = 255;
        }
    }
}

void drawHud(
    uint8_t* pixels,
    int width,
    int height,
    const ParticleSimulator& simulation,
    float fps,
    CanvasTool tool,
    ShapeForm form,
    ShapeStyle style
) {
    if (width < kPanelWidth || height < 32) return;

    fillRect(pixels, width, height, 0, 0, kPanelWidth, height, 16, 16, 18, 255);
    fillRect(pixels, width, height, kPanelWidth, 0, width - kPanelWidth, kToolbarHeight, 16, 16, 18, 255);

    BarButton buttons[8];
    const int count = toolbarButtons(buttons);
    for (int i = 0; i < count; ++i) {
        const BarButton& button = buttons[i];
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        toneColor(buttonTone(button, tool, form, style), r, g, b);
        fillRect(pixels, width, height, button.x, button.y, button.w, button.h, r, g, b, 255);
        const int chars = int(std::strlen(button.label));
        const int textW = chars * 6 - 1;
        drawText(pixels, width, height, button.x + std::max((button.w - textW) / 2, 2), button.y + 5, button.label);
    }

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
