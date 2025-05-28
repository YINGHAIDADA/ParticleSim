#include "render.h"
#include "render_utils.h"

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT messageType,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
		spdlog::info("[{}] {}", spdlog::fmt_lib::format(fg(spdlog::fmt_lib::terminal_color::red), "vulkan"), pCallbackData->pMessage);
	} else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
		spdlog::warn("[{}] {}", spdlog::fmt_lib::format(fg(spdlog::fmt_lib::terminal_color::red), "vulkan"), pCallbackData->pMessage);
	} else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
		spdlog::error("[{}] {}", spdlog::fmt_lib::format(fg(spdlog::fmt_lib::terminal_color::red), "vulkan"), pCallbackData->pMessage);
	}

	return vk::False;
}

void Render::createDebugMessenger() {
    if (!m_DebugLayer) return;

	spdlog::info("Creating Debug Messenger");
	m_Dispatcher.init(m_Instance, vkGetInstanceProcAddr);

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {};
	debugMessengerCreateInfo.pfnUserCallback = reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(debugCallback);
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
	
	spdlog::info("Debug Messenger was created successfully");
}