#include "render.h"

void Render::selectQueueFamilyIndexes() {
    spdlog::info("Select Queue Family indexes");

    std::vector<vk::QueueFamilyProperties> queueFamilies = m_PhysicalDevice.getQueueFamilyProperties();
    if (queueFamilies.empty()) throw std::runtime_error("Queue Family getting returned no results");

    uint32_t queueGraphicFamilyIndex = UINT32_MAX;
    uint32_t queuePresentFamilyIndex = UINT32_MAX;

	for (uint32_t i = 0; i < queueFamilies.size(); i++) {
		vk::QueueFamilyProperties& queueFamily = queueFamilies[i];

		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            if (queueGraphicFamilyIndex == UINT32_MAX) {
                spdlog::info("Graphic index {}", i);
                queueGraphicFamilyIndex = i;
            }

            if (queuePresentFamilyIndex == UINT32_MAX) {
                if (m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface).result == vk::Result::eSuccess) {
                    spdlog::info("Present index {}", i);
                    queuePresentFamilyIndex = i;
                }
            }
		}
	}

    if (queueGraphicFamilyIndex == UINT32_MAX && queuePresentFamilyIndex == UINT32_MAX) {
        throw std::runtime_error("Suitable Queue Family indexes were not found");
    }

    m_QueueGraphicFamilyIndex = queueGraphicFamilyIndex;
    
    m_QueuePresentFamilyIndex = queuePresentFamilyIndex == UINT32_MAX
        ? queueGraphicFamilyIndex
        : queuePresentFamilyIndex;
}