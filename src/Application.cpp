#include <chrono>
#include <thread>
#include <iostream>

#include "Application.h"
#include "Core/Window.h"
#include "Core/Inputs.h"
#include "Core/Renderer.h"
#include "Core/Context.h"
#include "Scene/Scene.h"
#include "Scene/Renderable.h"
#include "Assets/Mesh.h"
#include "Assets/Material.h"
#include "Core/Pipeline.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include "Core/ImGuiManager.h"


Application::Application(){
    m_window = std::make_unique<Window>("Vortex", 1280, 960);
    // 2. 创建输入系统 (依赖窗口)
    m_inputs = std::make_unique<Inputs>(m_window->getGLFWwindow());
    // 3. 创建渲染器 (依赖窗口)
    m_renderer = std::make_unique<Renderer>(m_window);
    // 4. 【关键】创建场景 (依赖渲染器已初始化)
    m_scene = std::make_unique<Scene>();
    // 5. 初始化场景内容 (依赖所有系统就绪)
    this->initializeScene();
    // 6. 注册窗口大小改变回调
    m_window->setFramebufferResizeCallback([this](uint32_t width, uint32_t height) {
        this->onWindowResize(width, height);
    });

    // 7. 注册 ImGui 绘制回调
    m_renderer->getImGuiManager()->setDrawCallback([this]() {
        auto& data = m_material->getData();
        ImGui::Begin("Material");
        ImGui::ColorEdit3("Albedo", &data.albedo.x);
        ImGui::SliderFloat("Metallic", &data.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &data.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("AO", &data.ao, 0.0f, 1.0f);
        ImGui::End();
    });
}
void Application::onWindowResize(uint32_t width, uint32_t height) {
    m_renderer->markFramebufferResized();
    std::cout << "Window resize requested: " << width << "x" << height << std::endl;
}

Application::~Application() {
    if (m_renderer && m_renderer->getContext()) {
        m_renderer->getContext()->getDevice().waitIdle();
    }
    m_scene.reset();
    m_material.reset();
    m_renderer.reset();
    m_inputs.reset();
    m_window.reset();
}

void Application::updateSceneFromInput(float deltaTime) {
    // WASD 移动控制
    if (this->m_inputs->isKeyPressed(Key::W)) {
        this->m_scene->getCamera().moveForward(deltaTime);
    }
    if (this->m_inputs->isKeyPressed(Key::A)) {
        this->m_scene->getCamera().moveLeft(deltaTime);
    }
    if (this->m_inputs->isKeyPressed(Key::S)) {
        this->m_scene->getCamera().moveBack(deltaTime);
    }
    if (this->m_inputs->isKeyPressed(Key::D)) {
        this->m_scene->getCamera().moveRight(deltaTime);
    }
    // 鼠标右键按下时旋转相机（自动鼠标捕获）
    if (this->m_inputs->isCursorCaptured()) {
        auto d = this->m_inputs->getMouseDelta();
        this->m_scene->getCamera().rotate(d.x, d.y);
    }
}

void Application::initializeScene(){
    // 设置相机
    m_scene->setCamera(std::make_unique<Camera>());

    int width, height;
    m_window->getFramebufferSize(&width, &height);
    m_scene->getCamera().setViewportSize(width, height);

    // 相机位置：放在原点前方，能够看到立方体
    m_scene->getCamera().setPosition({0.0f, 0.0f, 3.0f});  // 在z=3位置，看向z=-3的立方体
    // 获取 Context 指针
    Context* context = m_renderer->getContext();

    // 1. 创建 Mesh（从 assets 文件夹加载魔方模型）
    auto mesh = std::make_shared<Mesh>(context, "assets/Cube.obj");

    // 2. 准备材质数据（基础 PBR 参数）
    MaterialUBO materialData{
        .albedo = glm::vec3(1.0f),    // 白色基础色（纹理会覆盖）
        .metallic = 1.0f,                    // 非金属（塑料/油漆）
        .roughness = 0.1f,                   // 中等粗糙度
        .ao = 1.0f                          // 完整环境光遮蔽
    };

    // 3. 创建 Material（带完整 PBR 纹理）
    m_material = std::make_shared<Material>(
        context,
        PipelineType::Main,
        materialData,
        "assets/Cube_Diffuse.jpg",          // Albedo 纹理
        "assets/Cube_Normal.jpg",           // 法线贴图
        "assets/Cube_Glossyness.jpg",       // Metallic 贴图（glossiness 反转）
        "assets/Cube_Roughness.jpg"         // Roughness 纹理
    );

    // 4. 绑定纹理到描述符集
    m_material->bindToDescriptorSet(m_renderer->getDescriptorManager(),1,0);
    m_material->bindToDescriptorSet(m_renderer->getDescriptorManager(), 1, 1);

    // 5. 创建 Renderable 并添加到场景
    auto renderable1 = std::make_shared<Renderable>(mesh, m_material, 0);
    auto renderable2 = std::make_shared<Renderable>(mesh, m_material, 1);


    // 6. 设置变换（位置、旋转、缩放）
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 0.0f, -5.0f));  // 向后移动 3 单位
    modelMatrix = glm::rotate(modelMatrix, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f));  // 原始大小

    renderable1->updateTransform(modelMatrix);

    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 0.0f, -5.0f));  // 向后移动 3 单位
    modelMatrix = glm::rotate(modelMatrix, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f));  // 原始大小

    renderable2->updateTransform(modelMatrix);

    // 7. 添加到场景
    m_scene->addRenderable(renderable1);
    m_scene->addRenderable(renderable2);
}



void Application::run(){
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastFrameTime = startTime;

    // 帧率限制：60 FPS = 16.67ms per frame
    constexpr float TARGET_FPS = 60.0f;
    constexpr float FRAME_TIME = 1.0f / TARGET_FPS;  // 秒

    while (!this->m_window->shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();

        // 帧率限制：如果帧时间小于目标帧时间，休眠剩余时间
        float timeToWait = FRAME_TIME - deltaTime;
        if (timeToWait > 0.0f) {
            // 将秒转换为毫秒
            int millisecondsToWait = static_cast<int>(timeToWait * 1000.0f);
            std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsToWait));
        }

        // 重新获取时间（考虑休眠后）
        currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        //=========================================
        this->m_window->pollEvents();
        this->m_inputs->update();
        if (m_inputs->isKeyJustPressed(Key::Escape)) {
            break;
        }
        this->updateSceneFromInput(deltaTime);
        this->m_scene->updateAutoRotation(deltaTime, 30.0f);  // 每秒旋转30度
        this->m_renderer->render(this->m_scene);
        //=========================================
    }
}
