// main.cpp
#include <SDL3/SDL.h>
#include <stdint.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#include "ParticleSim.h"

ParticleSimulator::ParticleSimulator(int texyure_wdith, int texture_height) : particles(texyure_wdith*texture_height),
textureBuffer(texyure_wdith*texture_height) {}
ParticleSimulator::~ParticleSimulator() {
    
}

void ParticleSimulator::init() {

}

void ParticleSimulator::resetParticles() {
    std::fill(particles.begin(), particles.end(), Particle{MAT_EMPTY});
    std::fill(textureBuffer.begin(), textureBuffer.end(), Color{0,0,0,0});
}

void ParticleSimulator::update(float deltaTime) {
    
}
