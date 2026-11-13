#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>

#include "Hud.h"
#include "ParticleSim.h"
#include "logger.h"
#include "render/render.h"

namespace {

struct WindowDelete {
    void operator()(SDL_Window* window) const {
        if (window) SDL_DestroyWindow(window);
    }
};

std::optional<Material> materialFromKey(SDL_Keycode key) {
    switch (key) {
    case SDLK_1: return Material::Sand;
    case SDLK_2: return Material::Water;
    case SDLK_3: return Material::Salt;
    case SDLK_4: return Material::Wood;
    case SDLK_5: return Material::Fire;
    case SDLK_6: return Material::Gunpowder;
    case SDLK_7: return Material::Oil;
    case SDLK_8: return Material::Lava;
    case SDLK_9: return Material::Acid;
    case SDLK_0: return Material::Stone;
    default: return std::nullopt;
    }
}

class Application {
public:
    Application() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(SDL_GetError());
        }

        m_Window.reset(SDL_CreateWindow(
            "ParticleSim",
            kWindowWidth,
            kWindowHeight,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        ));
        if (!m_Window) {
            throw std::runtime_error(SDL_GetError());
        }

        int pixelWidth = 0;
        int pixelHeight = 0;
        SDL_GetWindowSizeInPixels(m_Window.get(), &pixelWidth, &pixelHeight);
        if (pixelWidth <= 0 || pixelHeight <= 0) {
            pixelWidth = kWindowWidth;
            pixelHeight = kWindowHeight;
        }
        m_WindowPixels = vk::Extent2D{static_cast<uint32_t>(pixelWidth), static_cast<uint32_t>(pixelHeight)};

        const int simWidth = std::max(pixelWidth / 2, 64);
        const int simHeight = std::max(pixelHeight / 2, 64);
        m_Simulation = std::make_unique<ParticleSimulator>(simWidth, simHeight);
        m_Render = std::make_unique<Render>(m_Window.get());
        m_Render->init(
            m_WindowPixels,
            static_cast<uint32_t>(m_Simulation->width()),
            static_cast<uint32_t>(m_Simulation->height())
        );

        SPDLOG_INFO(
            "左键绘制，右键擦除，1-0 切换材料，滚轮或 [ ] 调整笔刷，空格暂停，C 清空"
        );
        refreshTitle(0.f, true);
    }

    ~Application() {
        m_Render.reset();
        m_Window.reset();
        SDL_Quit();
    }

    void run() {
        auto last = std::chrono::steady_clock::now();
        while (m_Running) {
            const auto now = std::chrono::steady_clock::now();
            const float deltaTime = std::chrono::duration<float>(now - last).count();
            last = now;

            handleEvents();
            m_Simulation->update(deltaTime);
            if (m_Painting && !hudMaterialAt(m_MouseX, m_MouseY)) {
                m_Simulation->paint(m_MouseX, m_MouseY, m_Simulation->brushRadius(), brushMaterial());
            }
            m_Simulation->publish();
            drawHud(m_Simulation->pixelData(), m_Simulation->width(), m_Simulation->height(), *m_Simulation, m_Fps);
            m_Render->draw(std::span<const uint8_t>(m_Simulation->pixels(), m_Simulation->pixelBytes()), hudWidth());
            refreshTitle(deltaTime, false);
        }
    }

private:
    static constexpr int kWindowWidth = 1258;
    static constexpr int kWindowHeight = 848;

    std::unique_ptr<SDL_Window, WindowDelete> m_Window;
    std::unique_ptr<ParticleSimulator> m_Simulation;
    std::unique_ptr<Render> m_Render;
    vk::Extent2D m_WindowPixels;
    bool m_Running = true;
    bool m_Painting = false;
    bool m_Erasing = false;
    int m_MouseX = 0;
    int m_MouseY = 0;
    bool m_LeftDown = false;
    bool m_RightDown = false;
    float m_TitleTime = 0.f;
    int m_TitleFrames = 0;
    float m_Fps = 0.f;

    Material brushMaterial() const {
        return m_Erasing ? Material::Empty : m_Simulation->material();
    }

    void setMouse(float x, float y) {
        int windowWidth = 1;
        int windowHeight = 1;
        SDL_GetWindowSize(m_Window.get(), &windowWidth, &windowHeight);
        windowWidth = std::max(windowWidth, 1);
        windowHeight = std::max(windowHeight, 1);
        m_MouseX = std::clamp(int(x / float(windowWidth) * m_Simulation->width()), 0, m_Simulation->width() - 1);
        m_MouseY = std::clamp(int(y / float(windowHeight) * m_Simulation->height()), 0, m_Simulation->height() - 1);
    }

    void refreshTitle(float deltaSeconds, bool force) {
        if (deltaSeconds > 0.f) {
            m_TitleTime += deltaSeconds;
            ++m_TitleFrames;
            if (m_TitleTime >= 1.f) {
                m_Fps = m_TitleFrames / m_TitleTime;
                m_TitleTime = 0.f;
                m_TitleFrames = 0;
                force = true;
            }
        }
        if (!force) return;

        char title[160];
        std::snprintf(
            title,
            sizeof(title),
            "ParticleSim | %s | brush %d | %.0f fps%s",
            materialName(m_Simulation->material()),
            m_Simulation->brushRadius(),
            m_Fps,
            m_Simulation->running() ? "" : " | paused"
        );
        SDL_SetWindowTitle(m_Window.get(), title);
    }

    void handleEvents() {
        for (SDL_Event event; SDL_PollEvent(&event);) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                m_Running = false;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                const int width = std::max(event.window.data1, 0);
                const int height = std::max(event.window.data2, 0);
                vk::Extent2D next{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
                if (next.width == m_WindowPixels.width && next.height == m_WindowPixels.height) break;
                m_WindowPixels = next;
                m_Render->resize(m_WindowPixels);
                break;
            }
            case SDL_EVENT_KEY_DOWN: {
                if (event.key.repeat) break;
                if (const std::optional<Material> selected = materialFromKey(event.key.key)) {
                    m_Simulation->setMaterial(*selected);
                    refreshTitle(0.f, true);
                    break;
                }
                switch (event.key.key) {
                case SDLK_ESCAPE:
                    m_Running = false;
                    break;
                case SDLK_SPACE:
                    m_Simulation->setRunning(!m_Simulation->running());
                    refreshTitle(0.f, true);
                    break;
                case SDLK_C:
                    m_Simulation->clear();
                    break;
                case SDLK_LEFTBRACKET:
                    m_Simulation->setBrushRadius(m_Simulation->brushRadius() - 1);
                    refreshTitle(0.f, true);
                    break;
                case SDLK_RIGHTBRACKET:
                    m_Simulation->setBrushRadius(m_Simulation->brushRadius() + 1);
                    refreshTitle(0.f, true);
                    break;
                default:
                    break;
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                if (event.wheel.y == 0.f) break;
                const int step = event.wheel.y > 0.f ? 1 : -1;
                m_Simulation->setBrushRadius(m_Simulation->brushRadius() + step);
                refreshTitle(0.f, true);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                setMouse(event.button.x, event.button.y);
                if (const std::optional<Material> picked = hudMaterialAt(m_MouseX, m_MouseY)) {
                    m_Simulation->setMaterial(*picked);
                    refreshTitle(0.f, true);
                    break;
                }
                if (event.button.button == SDL_BUTTON_LEFT) m_LeftDown = true;
                if (event.button.button == SDL_BUTTON_RIGHT) m_RightDown = true;
                m_Erasing = m_RightDown;
                m_Painting = m_LeftDown || m_RightDown;
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == SDL_BUTTON_LEFT) m_LeftDown = false;
                if (event.button.button == SDL_BUTTON_RIGHT) m_RightDown = false;
                m_Erasing = m_RightDown;
                m_Painting = m_LeftDown || m_RightDown;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                setMouse(event.motion.x, event.motion.y);
                break;
            default:
                break;
            }
        }
    }
};

}  // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    setenv("MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS", "0", 1);
    setup_logger();
    try {
        Application app;
        app.run();
    } catch (const std::exception& error) {
        SPDLOG_ERROR("{}", error.what());
        return 1;
    }
    return 0;
}
