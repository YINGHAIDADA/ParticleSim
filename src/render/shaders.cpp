#include "render.h"
#include "render_utils.h"
#include "shader_path.h"

#include <stdexcept>
#include <vector>

namespace {

std::vector<char> loadFile(const char* filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open shader ") + filePath);
    }

    const auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    return buffer;
}

vk::ShaderModule createModule(vk::Device device, const std::vector<char>& code, const char* what) {
    vk::ShaderModuleCreateInfo info;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    return VK_ERROR_CHECK(device.createShaderModule(info), what);
}

}  // namespace

void Render::createShaderModules() {
    SPDLOG_INFO("Creating Shader Modules");

    m_VertexShader = createModule(m_LogicalDevice, loadFile(PARTICLESIM_VERT_SPV), "Vertex shader module creating caused an error");
    m_FragmentShader = createModule(m_LogicalDevice, loadFile(PARTICLESIM_FRAG_SPV), "Fragment shader module creating caused an error");
    m_BrightShader = createModule(m_LogicalDevice, loadFile(PARTICLESIM_BRIGHT_SPV), "Bright shader module creating caused an error");
    m_BlurShader = createModule(m_LogicalDevice, loadFile(PARTICLESIM_BLUR_SPV), "Blur shader module creating caused an error");

    SPDLOG_INFO("Shader Modules were created successfully");
}
