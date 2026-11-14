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
    void draw(std::span<const uint8_t> rgba, int hudPixels, float zoom, float centerU, float centerV);

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
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    };

    // Half-resolution bloom chain, one copy per frame so two frames never share an image.
    struct ColorTarget {
        vk::Image image;
        vk::DeviceMemory memory;
        vk::ImageView view;
        vk::Framebuffer framebuffer;
    };

    struct BloomFrame {
        ColorTarget bright;
        ColorTarget blurA;
        ColorTarget blurB;
        ColorTarget lightA;
        ColorTarget lightB;
        vk::DescriptorSet brightSet;
        vk::DescriptorSet blurFromBright;
        vk::DescriptorSet blurFromA;
        vk::DescriptorSet lightFromBright;
        vk::DescriptorSet lightFromA;
        vk::DescriptorSet compositeSet;
        vk::Extent2D brightExtent;
        vk::Extent2D blurExtent;
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
    vk::RenderPass m_OffscreenPass;
    vk::Pipeline m_Pipeline;
    vk::Pipeline m_BrightPipeline;
    vk::Pipeline m_BlurPipeline;
    vk::PipelineLayout m_PostLayout;
    vk::PipelineLayout m_CompositeLayout;
    vk::DescriptorSetLayout m_SampleLayout;
    vk::DescriptorSetLayout m_CompositeSetLayout;
    vk::DescriptorPool m_DescriptorPool;
    vk::Sampler m_Sampler;
    vk::Sampler m_LinearSampler;

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
    vk::ShaderModule m_BrightShader;
    vk::ShaderModule m_BlurShader;

    std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
    std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
    std::vector<vk::Fence> m_InFlightFences;
    std::vector<vk::Fence> m_ImageInFlight;
    uint32_t m_CurrentFrame = 0;

    uint32_t m_TexWidth = 0;
    uint32_t m_TexHeight = 0;
    std::array<FrameTarget, MAX_FRAMES_IN_FLIGHT> m_Targets{};
    std::array<BloomFrame, MAX_FRAMES_IN_FLIGHT> m_Bloom{};

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
    void createOffscreenPass();
    void createBloomTargets();
    void destroyBloomTargets();
    void destroyColorTarget(ColorTarget& target);
    void createColorTarget(ColorTarget& target, vk::Extent2D extent);
    void writeSampler(vk::DescriptorSet set, uint32_t binding, vk::Sampler sampler, vk::ImageView view);
    void drawFullscreen(vk::CommandBuffer commandBuffer, vk::Pipeline pipeline, vk::PipelineLayout layout,
                        vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent,
                        vk::DescriptorSet set, const void* push, uint32_t pushBytes, std::array<float, 4> clear);
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
