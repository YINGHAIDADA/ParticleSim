// main.cpp
#include "ParticleSim.h"
#include "Utilities.h"

ParticleSimulator::ParticleSimulator(int texture_wdith, int texture_height) {
    this->m_textureWidth = texture_wdith;
    this->m_textureHeight = texture_height;
    this->m_particles = new std::vector<Particle>(texture_wdith * texture_height, Particle{MAT_EMPTY});
    color_buffer = new Color[texture_wdith * texture_height];
}
ParticleSimulator::~ParticleSimulator() {
    delete m_particles;
    delete color_buffer;
    m_particles = nullptr;
}

void ParticleSimulator::init() {

}

void ParticleSimulator::resetParticles() {
    std::fill(m_particles->begin(), m_particles->end(), Particle{MAT_EMPTY});
}

void ParticleSimulator::update(float deltaTime) {
    m_deltaTime = deltaTime; // Update particles


}

Particle ParticleSimulator::particle_empty()
{
    Particle p = {0};
    p.id = mat_id_empty;
    p.color = mat_col_empty;
    return p;
}

Particle ParticleSimulator::particle_sand()
{
    Particle p = {0};
    p.id = mat_id_sand;
    // Random sand color
    float r = (float)(Utilities::random_val(0, 10)) / 10.f;
    p.color.r = (uint8_t)(Utilities::interp_linear(0.8f, 1.f, r) * 255.f);
    p.color.g = (uint8_t)(Utilities::interp_linear(0.5f, 0.6f, r) * 255.f);
    p.color.b = (uint8_t)(Utilities::interp_linear(0.2f, 0.25f, r) * 255.f);
    p.color.a = 255;
    return p;
}

Particle ParticleSimulator::particle_water()
{
    Particle p = {0};
    p.id = mat_id_water;
    float r = (float)(Utilities::random_val(0, 1)) / 2.f;
    p.color.r = (uint8_t)(Utilities::interp_linear(0.1f, 0.15f, r) * 255.f);
    p.color.g = (uint8_t)(Utilities::interp_linear(0.3f, 0.35f, r) * 255.f);
    p.color.b = (uint8_t)(Utilities::interp_linear(0.7f, 0.8f, r) * 255.f);
    p.color.a = 255;
    return p;
}

Particle ParticleSimulator::particle_salt()
{
    Particle p = {0};
    p.id = mat_id_salt;
    float r = (float)(Utilities::random_val(0, 1)) / 2.f;
    p.color.r = (uint8_t)(Utilities::interp_linear(0.9f, 1.0f, r) * 255.f);
    p.color.g = (uint8_t)(Utilities::interp_linear(0.8f, 0.85f, r) * 255.f);
    p.color.b = (uint8_t)(Utilities::interp_linear(0.8f, 0.9f, r) * 255.f);
    p.color.a = 255;
    return p;
}

Particle ParticleSimulator::particle_wood()
{
    Particle p = {0};
    p.id = mat_id_wood;
    float r = (float)(Utilities::random_val(0, 1)) / 2.f;
    p.color.r = (uint8_t)(Utilities::interp_linear(0.23f, 0.25f, r) * 255.f);
    p.color.g = (uint8_t)(Utilities::interp_linear(0.15f, 0.18f, r) * 255.f);
    p.color.b = (uint8_t)(Utilities::interp_linear(0.02f, 0.03f, r) * 255.f);
    p.color.a = 255;
    return p;
}

Particle ParticleSimulator::particle_fire()
{
    Particle p = {0};
    p.id = mat_id_fire;
    p.color = mat_col_fire;
    return p;
}

Particle ParticleSimulator::particle_lava()
{
    Particle p = {0};
    p.id = mat_id_lava;
    p.color = mat_col_fire;
    return p;
}

Particle ParticleSimulator::particle_smoke()
{
    Particle p = {0};
    p.id = mat_id_smoke;
    p.color = mat_col_smoke;
    return p;
}

Particle ParticleSimulator::particle_ember()
{
    Particle p = {0};
	p.id = mat_id_ember;
	p.color = mat_col_ember;
	return p;
}

Particle ParticleSimulator::particle_steam()
{
    Particle p = {0};
    p.id = mat_id_steam;
    p.color = mat_col_steam;
    return p;
}

Particle ParticleSimulator::particle_gunpowder()
{
    Particle p = {0};
    p.id = mat_id_gunpowder;
    float r = (float)(Utilities::random_val(0, 1)) / 2.f;
    p.color.r = (uint8_t)(Utilities::interp_linear(0.15f, 0.2f, r) * 255.f);
    p.color.g = (uint8_t)(Utilities::interp_linear(0.15f, 0.2f, r) * 255.f);
    p.color.b = (uint8_t)(Utilities::interp_linear(0.15f, 0.2f, r) * 255.f);
    p.color.a = 255;
    return p;
}

Particle ParticleSimulator::particle_oil()
{
    Particle p = {0};
    p.id = mat_id_oil;
    float r = (float)(Utilities::random_val(0, 1)) / 2.f;
    p.color.r = (uint8_t)(Utilities::interp_linear(0.12f, 0.15f, r) * 255.f);
    p.color.g = (uint8_t)(Utilities::interp_linear(0.10f, 0.12f, r) * 255.f);
    p.color.b = (uint8_t)(Utilities::interp_linear(0.08f, 0.10f, r) * 255.f);
    p.color.a = 255;
    return p;
}

Particle ParticleSimulator::particle_stone()
{
    Particle p = {0};
    p.id = mat_id_stone;
    float r = (float)(Utilities::random_val(0, 1)) / 2.f;
    p.color.r = (uint8_t)(Utilities::interp_linear(0.5f, 0.65f, r) * 255.f);
    p.color.g = (uint8_t)(Utilities::interp_linear(0.5f, 0.65f, r) * 255.f);
    p.color.b = (uint8_t)(Utilities::interp_linear(0.5f, 0.65f, r) * 255.f);
    p.color.a = 255;
    return p;
}

Particle ParticleSimulator::particle_acid()
{
    Particle p = {0};
    p.id = mat_id_acid;
    float r = (float)(Utilities::random_val(0, 1)) / 2.f;
    p.color.r = (uint8_t)(Utilities::interp_linear(0.05f, 0.06f, r) * 255.f);
    p.color.g = (uint8_t)(Utilities::interp_linear(0.8f, 0.85f, r) * 255.f);
    p.color.b = (uint8_t)(Utilities::interp_linear(0.1f, 0.12f, r) * 255.f);
    p.color.a = 200;
    return p;
}

void ParticleSimulator::update_particle_sim()
{
}

void ParticleSimulator::update_sand(uint32_t x, uint32_t y)
{
	// For water, same as sand, but we'll check immediate left and right as well
	uint32_t read_idx = compute_idx(x, y);
	Particle* p = &((*m_particles)[read_idx]);

    //更新速度
	p->velocity.y = utilities_clamp(p->velocity.y + (m_gravity * m_deltaTime), -10.f, 10.f);

	// 检查粒子是否可以直接下落，如果粒子下方是边界内且非空且不是水，则将速度减半
	if (in_bounds(x, y + 1) && !is_empty(x, y + 1) && get_particle_at(x, y + 1).id != mat_id_water) {
		p->velocity.y /= 2.f;
	}

    // 计算新的位置
	int32_t vi_x = x + (int32_t)p->velocity.x; 
	int32_t vi_y = y + (int32_t)p->velocity.y;

	uint32_t b_idx = compute_idx(x, y + 1);
	uint32_t br_idx = compute_idx(x + 1, y + 1);
	uint32_t bl_idx = compute_idx(x - 1, y + 1);

	int32_t lx, ly;

	Particle tmp_a = (*m_particles)[read_idx]; // 读取当前粒子
    

	// 检查是否可以交换位置Physics (using velocity)
	if (in_bounds(vi_x, vi_y) && (is_empty(vi_x, vi_y) ||
			((get_particle_at(vi_x, vi_y).id == mat_id_water) && 
			  !get_particle_at(vi_x, vi_y).updated && 
			   math_vec2_len(get_particle_at(vi_x, vi_y).velocity) - math_vec2_len(tmp_a.velocity) > 10.f))) {

		Particle tmp_b = get_particle_at(vi_x, vi_y); // 读取目标位置上的粒子

		// Try to throw water out
		if (tmp_b.id == mat_id_water) {

			int32_t rx = Utilities::random_val(-2, 2);
			tmp_b.velocity = (Vec2){(float)rx, -4.f};

			write_data(compute_idx(vi_x, vi_y), tmp_a);	

			for(int32_t i = -10; i < 0; ++i) {
				for (int32_t j = -10; j < 10; ++j) {
					if (is_empty(vi_x + j, vi_y + i)) {
						write_data(compute_idx(vi_x + j, vi_y + i), tmp_b);
						break;
					}	
				}
			}

			// Couldn't write there, so, uh, destroy it.
			write_data(read_idx, particle_empty());
		}
		else if (is_empty(vi_x, vi_y)) {
			write_data(compute_idx(vi_x, vi_y), tmp_a);
			write_data(read_idx, tmp_b);
		}
	}
	//Simple falling, changing the velocity here ruins everything. I need to redo this entire simulation.
	// else if (in_bounds(x, y + 1) && ((is_empty(x, y + 1) || ((*m_particles)[b_idx].id == mat_id_water)))) {
	// 	p->velocity.y += (m_gravity * m_deltaTime);
	// 	Particle tmp_b = get_particle_at(x, y + 1);
	// 	write_data(b_idx, *p);
	// 	write_data(read_idx, tmp_b);
	// }
	// else if (in_bounds(x - 1, y + 1) && ((is_empty(x - 1, y + 1) || (*m_particles)[bl_idx].id == mat_id_water))) {
	// 	p->velocity.x = is_in_liquid(x, y, &lx, &ly) ? 0.f : Utilities::random_val(0, 1) == 0 ? -1.f : 1.f;
	// 	p->velocity.y += (m_gravity * m_deltaTime);
	// 	Particle tmp_b = get_particle_at(x - 1, y + 1);
	// 	write_data(bl_idx, *p);
	// 	write_data(read_idx, tmp_b);
	// }
	// else if (in_bounds(x + 1, y + 1) && ((is_empty(x + 1, y + 1) || (*m_particles)[br_idx].id == mat_id_water))) {
	// 	p->velocity.x = is_in_liquid(x, y, &lx, &ly) ? 0.f : Utilities::random_val(0, 1) == 0 ? -1.f : 1.f;
	// 	p->velocity.y += (m_gravity * m_deltaTime);
	// 	Particle tmp_b = get_particle_at(x + 1, y + 1);
	// 	write_data(br_idx, *p);
	// 	write_data(read_idx, tmp_b);
	// }
	// else if (is_in_liquid(x, y, &lx, &ly) && Utilities::random_val(0, 10) == 0) {
	// 	Particle tmp_b = get_particle_at(lx, ly);
	// 	write_data(compute_idx(lx, ly), *p);
	// 	write_data(read_idx, tmp_b);
	// }
}

void ParticleSimulator::update_water(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_salt(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_fire(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_lava(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_smoke(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_ember(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_steam(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_gunpowder(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_oil(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_acid(uint32_t x, uint32_t y)
{
}

void ParticleSimulator::update_default(uint32_t x, uint32_t y)
{
}
