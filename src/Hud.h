#pragma once

#include "ParticleSim.h"

#include <cstdint>
#include <optional>

enum class CanvasTool : uint8_t { Brush, Hand, Shape };

enum class ToolbarHit : uint8_t { Brush, Hand, Shape, Rect, Circle, Solid, Stroke };

int hudWidth();
int toolbarHeight();
std::optional<Material> hudMaterialAt(int x, int y);
std::optional<ToolbarHit> toolbarAt(int x, int y);
bool shapeContains(int x, int y, int x0, int y0, int x1, int y1, ShapeForm form, ShapeStyle style, int stroke);
const uint8_t* hudGlyphRows(char letter);

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
);
void drawHud(
    uint8_t* pixels,
    int width,
    int height,
    const ParticleSimulator& simulation,
    float fps,
    CanvasTool tool,
    ShapeForm form,
    ShapeStyle style
);
