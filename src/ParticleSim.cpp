// main.cpp
#include <SDL3/SDL.h>
#include <stdint.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#include "ParticleSim.h"

ParticleSimulator::ParticleSimulator(int texture_wdith, int texture_height) {
    this->textureWidth = texture_wdith;
    this->textureHeight = texture_height;
    this->particles = new std::vector<Particle>(texture_wdith * texture_height, Particle{MAT_EMPTY});
}
ParticleSimulator::~ParticleSimulator() {
    delete particles;
    particles = nullptr;
}

void ParticleSimulator::init() {

}

void ParticleSimulator::resetParticles() {
    std::fill(particles->begin(), particles->end(), Particle{MAT_EMPTY});
}

void ParticleSimulator::update(float deltaTime) {
    
}
