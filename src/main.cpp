// main.cpp
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <cstring>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE  // 启用所有日志级别
#define SPDLOG_ENABLE_SOURCE_LOC  // 启用源位置支持
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>

#include "ParticleSim.h"
#include "render/render.h"
// 常量定义
static const int WINDOW_WIDTH = 1258;
static const int WINDOW_HEIGHT = 848;
static const int TEXTURE_WIDTH = 1258 >> 1;
static const int TEXTURE_HEIGHT = 848 >> 1;

class Application {
public:
    Application() {
        initSDL();
        initSimulation();
    }

    ~Application() {
        delete simulation;
        delete render;
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void run() {
        auto lastTime = std::chrono::high_resolution_clock::now();
        while (running) {
            handleEvents();
            
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            update(deltaTime);
            render->draw(windowSize);
            SDL_Delay(16); // 控制帧率
        }
    }

private:
    SDL_Window* window = NULL;
    Render* render = NULL;
    vk::Extent2D windowSize { WINDOW_WIDTH, WINDOW_HEIGHT };
    ParticleSimulator* simulation = NULL;
    bool running = true;

    void initSDL() {
        window = SDL_CreateWindow(
            "Vulkan Example",
            windowSize.width, windowSize.height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );
        render = new Render(window); // 创建渲染器
        render->init(windowSize); // 初始化渲染器
    }

    void initSimulation() {
        simulation = new ParticleSimulator(TEXTURE_WIDTH, TEXTURE_HEIGHT); // 原始尺寸的一半
    }

    void handleEvents() {
        
        for (SDL_Event event; SDL_PollEvent(&event);) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    vk::Extent2D newWindowSize {
                        static_cast<uint32_t>(event.window.data1),
                        static_cast<uint32_t>(event.window.data2)
                    };

                    if (windowSize == newWindowSize) break;
                    
                    windowSize = newWindowSize;
                    render->resize(newWindowSize);
                    break;
                }
                case SDL_EVENT_KEY_DOWN: {
                    switch (event.key.key) {
                        case SDLK_ESCAPE: {// 按下 ESC 键退出
                            running = false;
                            break;
                        }
                        default: {
                            SPDLOG_INFO("Key pressed: {}", SDL_GetKeyName(event.key.key));
                            break;
                        }
                    }
                }
            }
            // 处理其他事件
        }
    }

    void update(float deltaTime) {
        simulation->update(deltaTime);
    }
};

class SourceLocationFormatter : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& msg, 
                const std::tm&, 
                spdlog::memory_buf_t& dest) override {
        // 检查源位置是否有效
        if (msg.source.filename == nullptr || msg.source.filename[0] == '\0') {
            const char no_location[] = "[no location]";
            dest.append(no_location, no_location + sizeof(no_location) - 1); // 减去1是为了去掉末尾的'\0'
            return;
        }
        
        // 提取文件名（不含路径）
        const char* filename = msg.source.filename;
        if (const char* last_slash = std::strrchr(filename, '/')) {
            filename = last_slash + 1;
        } else if (const char* last_backslash = std::strrchr(filename, '\\')) {
            filename = last_backslash + 1;
        }
        
        // 提取函数名（简化）
        const char* funcname = msg.source.funcname ? msg.source.funcname : "unknown";
        std::string simplified_funcname = simplify_function_name(funcname);
        
        // 开始构建位置字符串
        dest.push_back('[');
        dest.append(filename, filename + strlen(filename));
        dest.push_back(':');
        
        // 添加行号
        auto line_str = std::to_string(msg.source.line);
        dest.append(line_str.data(), line_str.data() + line_str.size());
        dest.push_back(':');
        
        // 添加函数名
        dest.append(simplified_funcname.data(), 
                   simplified_funcname.data() + simplified_funcname.size());
        dest.push_back(']');
    }
    
    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<SourceLocationFormatter>();
    }
private:
    static std::string simplify_function_name(const char* full_name) {
        if (full_name == nullptr || *full_name == '\0') {
            return "unknown";
        }
        
        std::string funcname(full_name);
        
        // 移除函数参数列表
        size_t paren_pos = funcname.find('(');
        if (paren_pos != std::string::npos) {
            funcname = funcname.substr(0, paren_pos);
        }
        
        // 移除返回类型（如果存在）
        size_t space_pos = funcname.rfind(' ');
        if (space_pos != std::string::npos && space_pos + 1 < funcname.size()) {
            funcname = funcname.substr(space_pos + 1);
        }
        
        // 移除类作用域（可选）
        size_t scope_pos = funcname.rfind("::");
        if (scope_pos != std::string::npos && scope_pos + 2 < funcname.size()) {
            funcname = funcname.substr(scope_pos + 2);
        }
        
        return funcname;
    }
};
void setup_logger() {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs.log", true);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<SourceLocationFormatter>('@').set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %@ %v");
    
    console_sink->set_formatter(formatter->clone());
    file_sink->set_formatter(std::move(formatter));

    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("location_logger", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::info);
    
    spdlog::set_default_logger(logger);
    // spdlog::flush_every(std::chrono::seconds(1));
    
    // 测试日志
    SPDLOG_INFO("Logger initialized with location support");
}

int main(int argc, char* argv[]) {

    // auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs.txt", true);
    // auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // spdlog::logger logger("main", { file_sink, console_sink });
    // spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
    // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#:%!] %v");
    // spdlog::flush_every(std::chrono::seconds(1));
    setup_logger();

    Application app;
    app.run();
    return 0;
}
