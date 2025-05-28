#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "render/render.h"

int main() {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs.txt", true);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    spdlog::logger logger("main", { file_sink, console_sink });
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::flush_every(std::chrono::seconds(1));

    vk::Extent2D windowSize { 800, 600 };

    SDL_Window* window = SDL_CreateWindow(
        "Vulkan Example",
        windowSize.width, windowSize.height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    Render render(window);
    render.init(windowSize);

    for (bool running = true; running;) {
        render.draw(windowSize);
        
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
                    render.resize(newWindowSize);
                    break;
                }
            }
        }

        SDL_Delay(16);
    }
}