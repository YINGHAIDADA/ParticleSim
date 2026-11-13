#pragma once
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_NODISCARD_WARNINGS
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <spdlog/spdlog.h>

#include "render_utils.h"

class Render {
public:
    explicit Render(SDL_Window* window);
    ~Render();

    void init(vk::Extent2D& windowSize, uint32_t textureWidth, uint32_t textureHeight);
    void resize(vk::Extent2D& newWindowSize);
    void draw(std::span<const uint8_t> rgba);

private:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    // One upload target per frame in flight, so a copy never races a sample.
    struct FrameTarget {
        vk::Buffer staging;
        vk::DeviceMemory stagingMemory;
        void* mapped = nullptr;
        vk::Image image;
        vk::DeviceMemory imageMemory;
        vk::ImageView view;
        vk::DescriptorSet set;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    };

    bool m_DebugLayer = false;
    vk::detail::DispatchLoaderDynamic m_Dispatcher;
    vk::DebugUtilsMessengerEXT m_DebugMessenger;

    SDL_Window* m_Window = nullptr;
    vk::Instance m_Instance;
    vk::SurfaceKHR m_Surface;
    vk::PhysicalDevice m_PhysicalDevice;
    vk::Device m_LogicalDevice;
    vk::SwapchainKHR m_Swapchain;
    vk::CommandPool m_CommandPool;
    vk::RenderPass m_RenderPass;
    vk::Pipeline m_Pipeline;
    vk::PipelineLayout m_PipelineLayout;
    vk::DescriptorSetLayout m_DescriptorSetLayout;
    vk::DescriptorPool m_DescriptorPool;
    vk::Sampler m_Sampler;

    std::vector<vk::Image> m_SwapchainImages;
    std::vector<vk::ImageView> m_SwapchainImagesViews;
    size_t m_SwapchainImageCount = 0;
    std::vector<vk::CommandBuffer> m_CommandBuffers;
    std::vector<vk::Framebuffer> m_FrameBuffers;
    vk::Extent2D m_SwapchainExtent;
    vk::Extent2D m_WindowExtent;

    uint32_t m_QueueGraphicFamilyIndex = 0;
    vk::Queue m_GraphicQueue;
    uint32_t m_QueuePresentFamilyIndex = 0;
    vk::Queue m_PresentQueue;

    vk::ShaderModule m_VertexShader;
    vk::ShaderModule m_FragmentShader;

    std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
    std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
    std::vector<vk::Fence> m_InFlightFences;
    std::vector<vk::Fence> m_ImageInFlight;
    uint32_t m_CurrentFrame = 0;

    uint32_t m_TexWidth = 0;
    uint32_t m_TexHeight = 0;
    std::array<FrameTarget, MAX_FRAMES_IN_FLIGHT> m_Targets{};

    void createInstance();
    void createDebugMessenger();
    void createSurface();
    void selectPhysicalDevice();
    void selectQueueFamilyIndexes();
    void createLogicalDevice();
    void createSwapchain(vk::Extent2D& windowSize);
    void recreateSwapchain(vk::Extent2D& windowSize);
    void destroyFramebuffersAndViews();
    void selectSwapchainResources();

    void createShaderModules();
    void createPipeline();
    void createPresentTargets();
    void destroyPresentTargets();
    void transition(vk::CommandBuffer commandBuffer, FrameTarget& target, vk::ImageLayout next);

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
    void createCommandPool();
    void createCommandBuffers();
    void createRenderPass();
    void createFrameBuffers();
    void createSyncObjects();
    void createPresentSemaphores();
    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer& commandBuffer);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
};
