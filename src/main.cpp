// main.cpp
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "ParticleSim.h"
#include "render/render.h"
// 常量定义
const int WINDOW_WIDTH = 1258;
const int WINDOW_HEIGHT = 848;
const int TEXTURE_WIDTH = 1258 >> 1;
const int TEXTURE_HEIGHT = 848 >> 1;

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
    vk::Extent2D windowSize { 800, 600 };
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
                            spdlog::info("Key pressed: {}", SDL_GetKeyName(event.key.key));
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

int main(int argc, char* argv[]) {

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs.txt", true);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    spdlog::logger logger("main", { file_sink, console_sink });
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::flush_every(std::chrono::seconds(1));

    Application app;
    app.run();
    return 0;
}
