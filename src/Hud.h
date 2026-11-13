#pragma once

#include "ParticleSim.h"

#include <cstdint>
#include <optional>

int hudWidth();
std::optional<Material> hudMaterialAt(int x, int y);
const uint8_t* hudGlyphRows(char letter);

void drawHud(uint8_t* pixels, int width, int height, const ParticleSimulator& simulation, float fps);
