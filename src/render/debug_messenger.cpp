#include "render.h"
#include "render_utils.h"

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT messageType,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
		spdlog::error("[vulkan] {}", pCallbackData->pMessage);
	} else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
		spdlog::warn("[vulkan] {}", pCallbackData->pMessage);
	} else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
		SPDLOG_INFO("[vulkan] {}", pCallbackData->pMessage);
	}

	return vk::False;
}

void Render::createDebugMessenger() {
    if (!m_DebugLayer) return;

	SPDLOG_INFO("Creating Debug Messenger");
	m_Dispatcher.init(m_Instance, vkGetInstanceProcAddr);

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {};
	debugMessengerCreateInfo.pfnUserCallback = debugCallback;
	debugMessengerCreateInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
	debugMessengerCreateInfo.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

	m_DebugMessenger = VK_ERROR_CHECK(
		m_Instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo, nullptr, m_Dispatcher),
		"Debug Messenger creating caused an error"
	);
	
	SPDLOG_INFO("Debug Messenger was created successfully");
}