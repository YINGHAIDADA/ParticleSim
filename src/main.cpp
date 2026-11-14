#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

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

struct CursorDelete {
    void operator()(SDL_Cursor* cursor) const {
        if (cursor) SDL_DestroyCursor(cursor);
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
    case SDLK_P: return Material::Plant;
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

        m_HomeX = m_CamX = m_Simulation->viewX();
        m_HomeY = m_CamY = m_Simulation->viewY();
        m_HomeZoom = m_Zoom = m_Simulation->viewZoom();
        m_BrushCursor.reset(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
        m_HandCursor.reset(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER));
        applyCursor();

        SPDLOG_INFO(
            "左键绘制，右键擦除，上方工具栏切换画笔、手型和形状，H 手型，B 画笔，S 形状，R 重置视角，1-0 切换材料，P 植物，滚轮或 [ ] 调整笔刷，空格暂停，C 清空"
        );
        refreshTitle(0.f, true);
    }

    ~Application() {
        m_BrushCursor.reset();
        m_HandCursor.reset();
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
            int simX = 0;
            int simY = 0;
            if (m_Tool == CanvasTool::Brush && m_Painting && simPixel(m_PointerX, m_PointerY, simX, simY) &&
                !overHud(m_PointerX, m_PointerY)) {
                m_Simulation->paint(simX, simY, m_Simulation->brushRadius(), brushMaterial());
            }
            m_Simulation->setView(m_CamX, m_CamY, m_Zoom);
            m_Simulation->publish();
            if (m_Shaping) {
                uint8_t previewR = 180;
                uint8_t previewG = 80;
                uint8_t previewB = 80;
                if (!m_ShapeErase) m_Simulation->materialRgb(m_Simulation->material(), previewR, previewG, previewB);
                drawShapePreview(
                    m_Simulation->pixelData(),
                    m_Simulation->width(),
                    m_Simulation->height(),
                    m_CamX,
                    m_CamY,
                    m_Zoom,
                    m_ShapeX0,
                    m_ShapeY0,
                    m_ShapeX1,
                    m_ShapeY1,
                    m_Form,
                    m_Style,
                    m_Simulation->brushRadius(),
                    previewR,
                    previewG,
                    previewB
                );
            }
            drawHud(
                m_Simulation->pixelData(),
                m_Simulation->width(),
                m_Simulation->height(),
                *m_Simulation,
                m_Fps,
                m_Tool,
                m_Form,
                m_Style
            );
            const float panel = panelU();
            m_Render->draw(
                std::span<const uint8_t>(m_Simulation->pixels(), m_Simulation->pixelBytes()),
                hudWidth(),
                toolbarHeight(),
                1.f,
                (panel + 1.f) * 0.5f,
                0.5f
            );
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
    bool m_Panning = false;
    CanvasTool m_Tool = CanvasTool::Brush;
    ShapeForm m_Form = ShapeForm::Rect;
    ShapeStyle m_Style = ShapeStyle::Solid;
    bool m_Shaping = false;
    bool m_ShapeErase = false;
    int m_ShapeX0 = 0;
    int m_ShapeY0 = 0;
    int m_ShapeX1 = 0;
    int m_ShapeY1 = 0;
    float m_Zoom = 1.f;
    float m_CamX = 0.f;
    float m_CamY = 0.f;
    float m_HomeX = 0.f;
    float m_HomeY = 0.f;
    float m_HomeZoom = 1.f;
    float m_GrabU = 0.f;
    float m_GrabV = 0.f;
    float m_PointerX = 0.f;
    float m_PointerY = 0.f;
    bool m_LeftDown = false;
    bool m_RightDown = false;
    std::unique_ptr<SDL_Cursor, CursorDelete> m_BrushCursor;
    std::unique_ptr<SDL_Cursor, CursorDelete> m_HandCursor;
    float m_TitleTime = 0.f;
    int m_TitleFrames = 0;
    float m_Fps = 0.f;

    Material brushMaterial() const {
        return m_Erasing ? Material::Empty : m_Simulation->material();
    }

    struct WindowSpan {
        int width = 1;
        int height = 1;
    };

    struct ScreenPoint {
        float screenU = 0.f;
        float screenV = 0.f;
        float localX = 0.f;
        float worldX = 0.f;
        float worldY = 0.f;
        bool onCanvas = false;
    };

    WindowSpan windowSpan() const {
        int width = 1;
        int height = 1;
        SDL_GetWindowSize(m_Window.get(), &width, &height);
        return {std::max(width, 1), std::max(height, 1)};
    }

    float panelU() const {
        return hudWidth() / float(std::max(m_Simulation->width(), 1));
    }

    ScreenPoint mapPointer(float x, float y) const {
        const WindowSpan window = windowSpan();
        const int texW = std::max(m_Simulation->width(), 1);
        const int texH = std::max(m_Simulation->height(), 1);
        const float hud = float(hudWidth());
        const float bar = float(toolbarHeight());
        const float zoom = std::max(m_Zoom, 0.05f);
        const float canvasW = std::max(float(texW) - hud, 1.f);
        const float canvasH = std::max(float(texH) - bar, 1.f);
        ScreenPoint point;
        point.screenU = x / float(window.width);
        point.screenV = y / float(window.height);
        const float texX = point.screenU * float(texW);
        const float texY = point.screenV * float(texH);
        point.localX = (texX - hud) / canvasW;
        const float localY = (texY - bar) / canvasH;
        point.worldX = m_CamX + (point.localX - 0.5f) * canvasW / zoom;
        point.worldY = m_CamY + (localY - 0.5f) * canvasH / zoom;
        point.onCanvas = texX >= hud && texY >= bar && texX < float(texW) && texY < float(texH);
        return point;
    }

    void lookAt(float x, float y, float worldX, float worldY) {
        const WindowSpan window = windowSpan();
        const int texW = std::max(m_Simulation->width(), 1);
        const int texH = std::max(m_Simulation->height(), 1);
        const float hud = float(hudWidth());
        const float bar = float(toolbarHeight());
        const float zoom = std::max(m_Zoom, 0.05f);
        const float canvasW = std::max(float(texW) - hud, 1.f);
        const float canvasH = std::max(float(texH) - bar, 1.f);
        const float texX = x / float(window.width) * float(texW);
        const float texY = y / float(window.height) * float(texH);
        const float localX = (texX - hud) / canvasW;
        const float localY = (texY - bar) / canvasH;
        m_CamX = worldX - (localX - 0.5f) * canvasW / zoom;
        m_CamY = worldY - (localY - 0.5f) * canvasH / zoom;
    }

    void hudPixel(float x, float y, int& tx, int& ty) const {
        const WindowSpan window = windowSpan();
        tx = std::clamp(int(x / float(window.width) * m_Simulation->width()), 0, m_Simulation->width() - 1);
        ty = std::clamp(int(y / float(window.height) * m_Simulation->height()), 0, m_Simulation->height() - 1);
    }

    bool overHud(float x, float y) const {
        int tx = 0;
        int ty = 0;
        hudPixel(x, y, tx, ty);
        return hudMaterialAt(tx, ty).has_value() || toolbarAt(tx, ty).has_value();
    }

    bool simPixel(float x, float y, int& simX, int& simY) const {
        const ScreenPoint point = mapPointer(x, y);
        if (!point.onCanvas) return false;
        simX = int(std::floor(point.worldX));
        simY = int(std::floor(point.worldY));
        return true;
    }

    void applyCursor() {
        SDL_Cursor* cursor = nullptr;
        switch (m_Tool) {
        case CanvasTool::Brush:
        case CanvasTool::Shape:
            cursor = m_BrushCursor.get();
            break;
        case CanvasTool::Hand:
            cursor = m_HandCursor.get();
            break;
        default: {
            const CanvasTool unhandled = m_Tool;
            throw std::logic_error("unhandled canvas tool " + std::to_string(int(unhandled)));
        }
        }
        if (cursor) SDL_SetCursor(cursor);
    }

    void setTool(CanvasTool tool) {
        m_Shaping = false;
        m_Panning = false;
        m_Painting = false;
        if (m_Tool == tool) return;
        m_Tool = tool;
        applyCursor();
        refreshTitle(0.f, true);
    }

    void selectToolbar(ToolbarHit hit) {
        switch (hit) {
        case ToolbarHit::Brush:
            setTool(CanvasTool::Brush);
            break;
        case ToolbarHit::Hand:
            setTool(CanvasTool::Hand);
            break;
        case ToolbarHit::Shape:
            setTool(CanvasTool::Shape);
            break;
        case ToolbarHit::Rect:
            m_Form = ShapeForm::Rect;
            setTool(CanvasTool::Shape);
            refreshTitle(0.f, true);
            break;
        case ToolbarHit::Circle:
            m_Form = ShapeForm::Circle;
            setTool(CanvasTool::Shape);
            refreshTitle(0.f, true);
            break;
        case ToolbarHit::Solid:
            m_Style = ShapeStyle::Solid;
            setTool(CanvasTool::Shape);
            refreshTitle(0.f, true);
            break;
        case ToolbarHit::Stroke:
            m_Style = ShapeStyle::Stroke;
            setTool(CanvasTool::Shape);
            refreshTitle(0.f, true);
            break;
        default: {
            const ToolbarHit unhandled = hit;
            throw std::logic_error("unhandled toolbar hit " + std::to_string(int(unhandled)));
        }
        }
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
        switch (m_Tool) {
        case CanvasTool::Hand:
            std::snprintf(
                title,
                sizeof(title),
                "ParticleSim | hand | zoom %.2f | %.0f fps%s",
                m_Zoom,
                m_Fps,
                m_Simulation->running() ? "" : " | paused"
            );
            break;
        case CanvasTool::Brush:
            std::snprintf(
                title,
                sizeof(title),
                "ParticleSim | %s | brush %d | %.0f fps%s",
                materialName(m_Simulation->material()),
                m_Simulation->brushRadius(),
                m_Fps,
                m_Simulation->running() ? "" : " | paused"
            );
            break;
        case CanvasTool::Shape:
            std::snprintf(
                title,
                sizeof(title),
                "ParticleSim | shape %s %s | width %d | %.0f fps%s",
                m_Form == ShapeForm::Circle ? "circle" : "rect",
                m_Style == ShapeStyle::Stroke ? "stroke" : "fill",
                m_Simulation->brushRadius(),
                m_Fps,
                m_Simulation->running() ? "" : " | paused"
            );
            break;
        default: {
            const CanvasTool unhandled = m_Tool;
            throw std::logic_error("unhandled canvas tool " + std::to_string(int(unhandled)));
        }
        }
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
                case SDLK_B:
                    setTool(CanvasTool::Brush);
                    break;
                case SDLK_H:
                    setTool(CanvasTool::Hand);
                    break;
                case SDLK_S:
                    setTool(CanvasTool::Shape);
                    break;
                case SDLK_R:
                    m_CamX = m_HomeX;
                    m_CamY = m_HomeY;
                    m_Zoom = m_HomeZoom;
                    m_Panning = false;
                    refreshTitle(0.f, true);
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
                if (m_Tool == CanvasTool::Hand) {
                    const ScreenPoint before = mapPointer(m_PointerX, m_PointerY);
                    if (!before.onCanvas) break;
                    const float factor = event.wheel.y > 0.f ? 1.12f : 1.f / 1.12f;
                    m_Zoom = std::clamp(m_Zoom * factor, 0.2f, 16.f);
                    lookAt(m_PointerX, m_PointerY, before.worldX, before.worldY);
                    refreshTitle(0.f, true);
                    break;
                }
                const int step = event.wheel.y > 0.f ? 1 : -1;
                m_Simulation->setBrushRadius(m_Simulation->brushRadius() + step);
                refreshTitle(0.f, true);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                m_Simulation->cancelIntro();
                m_PointerX = event.button.x;
                m_PointerY = event.button.y;
                {
                    int hudX = 0;
                    int hudY = 0;
                    hudPixel(m_PointerX, m_PointerY, hudX, hudY);
                    if (const std::optional<ToolbarHit> hit = toolbarAt(hudX, hudY)) {
                        selectToolbar(*hit);
                        break;
                    }
                    if (const std::optional<Material> picked = hudMaterialAt(hudX, hudY)) {
                        m_Simulation->setMaterial(*picked);
                        if (m_Tool == CanvasTool::Hand) setTool(CanvasTool::Brush);
                        else refreshTitle(0.f, true);
                        break;
                    }
                }
                if (m_Tool == CanvasTool::Shape) {
                    const ScreenPoint point = mapPointer(m_PointerX, m_PointerY);
                    if (!point.onCanvas) break;
                    if (event.button.button != SDL_BUTTON_LEFT && event.button.button != SDL_BUTTON_RIGHT) break;
                    m_Shaping = true;
                    m_ShapeErase = event.button.button == SDL_BUTTON_RIGHT;
                    m_ShapeX0 = m_ShapeX1 = int(std::floor(point.worldX));
                    m_ShapeY0 = m_ShapeY1 = int(std::floor(point.worldY));
                    break;
                }
                if (m_Tool == CanvasTool::Hand) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        const ScreenPoint point = mapPointer(m_PointerX, m_PointerY);
                        if (point.onCanvas) {
                            m_Panning = true;
                            m_GrabU = point.worldX;
                            m_GrabV = point.worldY;
                        }
                    }
                    break;
                }
                if (event.button.button == SDL_BUTTON_LEFT) m_LeftDown = true;
                if (event.button.button == SDL_BUTTON_RIGHT) m_RightDown = true;
                m_Erasing = m_RightDown;
                m_Painting = m_LeftDown || m_RightDown;
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (m_Shaping &&
                    ((m_ShapeErase && event.button.button == SDL_BUTTON_RIGHT) ||
                     (!m_ShapeErase && event.button.button == SDL_BUTTON_LEFT))) {
                    m_Simulation->paintShape(
                        m_ShapeX0,
                        m_ShapeY0,
                        m_ShapeX1,
                        m_ShapeY1,
                        m_Form,
                        m_Style,
                        m_Simulation->brushRadius(),
                        m_ShapeErase ? Material::Empty : m_Simulation->material()
                    );
                    m_Shaping = false;
                }
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_LeftDown = false;
                    m_Panning = false;
                }
                if (event.button.button == SDL_BUTTON_RIGHT) m_RightDown = false;
                m_Erasing = m_RightDown;
                m_Painting = m_LeftDown || m_RightDown;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                m_PointerX = event.motion.x;
                m_PointerY = event.motion.y;
                if (m_Shaping) {
                    const ScreenPoint point = mapPointer(m_PointerX, m_PointerY);
                    if (point.onCanvas) {
                        m_ShapeX1 = int(std::floor(point.worldX));
                        m_ShapeY1 = int(std::floor(point.worldY));
                    }
                }
                if (m_Panning) lookAt(m_PointerX, m_PointerY, m_GrabU, m_GrabV);
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
