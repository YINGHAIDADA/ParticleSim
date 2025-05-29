#pragma once
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_NODISCARD_WARNINGS
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>
#include <optional>
#include <sstream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/color.h>

#include "render_utils.h"

// 纹理资源结构体
struct Texture {
    vk::Image image;
    vk::ImageView view;
    vk::DeviceMemory memory;
    vk::Sampler sampler;
    vk::Framebuffer framebuffer;
    uint32_t width;
    uint32_t height;
};

// 添加GlyphInfo结构定义
struct GlyphInfo {
    float x0, y0;   // 纹理坐标起始位置
    float x1, y1;   // 纹理坐标结束位置
    float xoff;     // X偏移
    float yoff;     // Y偏移
    float xadvance; // 字符间距
};

// 文本顶点结构
struct TextVertex {
    float position[2]; // x, y
    float uv[2];       // u, v
    
    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(TextVertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }
    
    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};
        
        // 位置属性
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = offsetof(TextVertex, position);
        
        // UV属性
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[1].offset = offsetof(TextVertex, uv);
        
        return attributeDescriptions;
    }
};

class Render {
    public:
        Render(SDL_Window* window);
        ~Render();

        void init(vk::Extent2D& windowSize);
        void resize(vk::Extent2D& newWindowSize);
        void draw(vk::Extent2D& windowSize);
        void draw_custom(vk::Extent2D& windowSize);
        void draw_offscreen(vk::Extent2D& windowSize);
        void draw_postprocess(vk::Extent2D& windowSize);
        void draw_ui(vk::Extent2D& windowSize);
        void drawText(vk::CommandBuffer cmd, const std::string& text, 
                 float x, float y, float scale, const float color[4]);

        // 设置纹理数据
        void setTextureData(uint8_t* data, uint32_t width, uint32_t height) {
            g_texture_buffer = data;
            g_texture_width = width;
            g_texture_height = height;
        }

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
        vk::Extent2D m_SwapchainExtent;

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
        void recreateSwapchain(vk::Extent2D& windowSize);
        void selectSwapchainResources();

        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
        void createShaderModules();
        vk::ShaderModule createShaderModule(const std::vector<char>& code);
        std::vector<char> readFile(const std::string &filename);

        void createCommandPool();
        void createCommandBuffers();
        void createRenderPass();
        void createFrameBuffers(vk::Extent2D& windowSize);
        void createSyncObjects();
        
        void createPipeline();
        void createUIPipeline();

        void initResources();
        vk::CommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(vk::CommandBuffer &commandBuffer);
        void createTexture(Texture& tex, uint32_t width, uint32_t height, 
                          vk::Format format, vk::ImageUsageFlags usage, 
                          void* initialData);
        void createRenderTarget(Texture& rt, uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage);
        void createOffscreenRenderPass();
        void createMainPipeline();
        void construct_font_data();
        

        void applyPostEffect(vk::CommandBuffer cmd, vk::Pipeline pipeline, vk::DescriptorSet descriptorSet);
        void applyBlurPass(vk::CommandBuffer cmd, vk::Pipeline pipeline, vk::DescriptorSet descriptorSet);
        void transitionImageLayout(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

        vk::Semaphore m_ImageAvailableSemaphore;
        vk::Semaphore m_SubmitSemaphore;
        vk::Fence m_RenderFinishedFence;

        // 纹理相关
        uint8_t* g_texture_buffer = nullptr;
        uint8_t* g_ui_buffer = nullptr;
        uint32_t g_texture_width = 0;
        uint32_t g_texture_height = 0;

        Texture g_tex;  // 主纹理
        Texture g_tex_ui; // UI纹理
        Texture g_rt;    // 渲染目标纹理
        Texture g_tex_font; // 字体纹理
        std::unordered_map<char, GlyphInfo> m_glyphs; // 字体信息

        // 离屏渲染目标
        vk::RenderPass m_offscreen_render_pass;
        vk::Framebuffer m_OffscreenFramebuffer;
        vk::Image m_OffscreenImage;
        vk::ImageView m_OffscreenImageView;
        vk::DeviceMemory m_OffscreenImageMemory;

        // 后处理相关

        // 管线布局
        vk::Pipeline m_ui_pipeline; // UI管线
        vk::PipelineLayout m_pipeline_layout;
        

        // 顶点/索引缓冲区
        vk::Buffer m_VertexBuffer;
        vk::DeviceMemory m_VertexBufferMemory;
        vk::Buffer m_IndexBuffer;
        vk::DeviceMemory m_IndexBufferMemory;
        // 添加动态顶点缓冲区
        vk::Buffer m_DynamicVertexBuffer;
        vk::DeviceMemory m_DynamicVertexBufferMemory;
        
        // 描述符集和布局
        vk::DescriptorSetLayout m_descriptor_set_layout;
        vk::DescriptorPool m_descriptor_pool;
        vk::DescriptorSet m_main_descriptor_set;
        vk::DescriptorSet m_ui_descriptor_set;
};