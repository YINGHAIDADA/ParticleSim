#pragma once

#include "ParticleSim.h"

#include <cstdint>
#include <optional>

enum class CanvasTool : uint8_t { Brush, Hand };

int hudWidth();
std::optional<Material> hudMaterialAt(int x, int y);
std::optional<CanvasTool> hudToolAt(int x, int y);
const uint8_t* hudGlyphRows(char letter);

void drawHud(uint8_t* pixels, int width, int height, const ParticleSimulator& simulation, float fps, CanvasTool tool);
