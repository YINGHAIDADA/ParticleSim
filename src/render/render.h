#pragma once
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_NODISCARD_WARNINGS
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>
#include <optional>
#include <unordered_set>
#include <sstream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/color.h>
#include "render_utils.h"

class Render {
    public:
        Render(SDL_Window* window);
        ~Render();

        void init(vk::Extent2D& windowSize);
        void resize(vk::Extent2D& newWindowSize);
        void draw(vk::Extent2D& windowSize);

    private:
        bool m_DebugLayer = false;
        vk::DispatchLoaderDynamic m_Dispatcher;
        vk::DebugUtilsMessengerEXT m_DebugMessenger;

        SDL_Window* m_Window;
        vk::Instance m_Instance;
        vk::SurfaceKHR m_Surface;
        vk::PhysicalDevice m_PhysicalDevice;
        vk::Device m_LogicalDevice;
        vk::SwapchainKHR m_Swapchain;
        vk::CommandPool m_CommandPool;
        vk::RenderPass m_RenderPass;
        vk::Pipeline m_Pipeline;

        std::vector<vk::Image> m_SwapchainImages;
        std::vector<vk::ImageView> m_SwapchainImagesViews;
        std::vector<vk::CommandBuffer> m_CommandBuffers;
        std::vector<vk::Framebuffer> m_FrameBuffers;

        uint32_t m_QueueGraphicFamilyIndex;
        vk::Queue m_GraphicQueue;
        uint32_t m_QueuePresentFamilyIndex;
        vk::Queue m_PresentQueue;

        vk::ShaderModule m_VertexShader;
        vk::ShaderModule m_FragmentShader;

        void createInstance();
        void createDebugMessenger();
        void createSurface();
        void selectPhysicalDevice();
        void selectQueueFamilyIndexes();
        void createLogicalDevice();
        void createSwapchain(vk::Extent2D& windowSize);
        void selectSwapchainResources();
        void createShaderModules();
        void createCommandPool();
        void createCommandBuffers();
        void createRenderPass();
        void createFrameBuffers(vk::Extent2D& windowSize);
        void createSyncObjects();
        
        void createPipeline();

        vk::Semaphore m_ImageAvailableSemaphore;
        vk::Semaphore m_SubmitSemaphore;
        vk::Fence m_RenderFinishedFence;
};