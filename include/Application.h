#pragma once
#include <memory>
#include <cstdint>

class Scene;
class Inputs;
class Window;
class Renderer;
struct MaterialUBO;

class Application{
private:
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Inputs> m_inputs;
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::shared_ptr<class Material> m_material;

private:
    // 窗口大小改变回调
    void onWindowResize(uint32_t width, uint32_t height);

    void updateSceneFromInput(float deltaTime);
    void initializeScene();
public:
    Application();
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void run();
};
