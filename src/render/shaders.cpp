#include "render.h"
#include "render_utils.h"

std::vector<char> loadFile(const char* filePath) {
    std::ifstream file = std::ifstream(filePath, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
		std::ostringstream os;
		os << "Failed to open file" << filePath;

		throw std::runtime_error(os.str());
	}

    size_t fileSize { static_cast<size_t>(file.tellg()) };
    std::vector<char> buffer(fileSize);

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void Render::createShaderModules() {
	spdlog::info("Creating Shader Modules");

    std::vector<char> vertexContent = loadFile("../../shaders/basic_vert.spv");
	std::vector<char> fragmentContent = loadFile("../../shaders/basic_frag.spv");

	vk::ShaderModuleCreateInfo vertexInfo {};
	vertexInfo.codeSize = vertexContent.size();
	vertexInfo.pCode = reinterpret_cast<uint32_t*>(vertexContent.data());

	vk::ShaderModuleCreateInfo fragmentInfo {};
	fragmentInfo.codeSize = fragmentContent.size();
	fragmentInfo.pCode = reinterpret_cast<uint32_t*>(fragmentContent.data());

	m_VertexShader = VK_ERROR_CHECK(
		m_LogicalDevice.createShaderModule(vertexInfo),
		"Vertex Shader Module creating caused an error"
	);

	m_FragmentShader = VK_ERROR_CHECK(
		m_LogicalDevice.createShaderModule(fragmentInfo),
		"Fragment Shader Module creating caused an error"
	);

	spdlog::info("Shader Modules were created successfully");
}

// 创建着色器模块
vk::ShaderModule Render::createShaderModule(const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    return VK_ERROR_CHECK(m_LogicalDevice.createShaderModule(createInfo), "Shader Module creating caused an error");
}
// 读取着色器文件
std::vector<char> Render::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
}