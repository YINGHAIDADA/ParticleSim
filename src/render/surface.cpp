#include "render.h"

void Render::createSurface() {
    SPDLOG_INFO("Creating Surface");
    VkSurfaceKHR surface;

    if (!SDL_Vulkan_CreateSurface(m_Window, m_Instance, nullptr, &surface)) {
        throw std::runtime_error("Surface creating caused an error");
    }

    m_Surface = surface;
    SPDLOG_INFO("Surface was created successfully");
}