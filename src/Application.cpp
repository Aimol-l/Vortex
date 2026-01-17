#include "Application.h"
#include "Core/Window.h"
#include "Core/Inputs.h"
#include "Core/Renderer.h"
#include <iostream>

void Application::onWindowResize(uint32_t width, uint32_t height) {
    m_renderer->m_framebufferResized = true;
    std::cout << "Window resize requested: " << width << "x" << height << std::endl;
}

Application::~Application() {
    if (m_renderer && m_renderer->getContext()) {
        m_renderer->getContext()->getDevice().waitIdle();
    }
    m_scene.reset();
    m_renderer.reset();
    m_inputs.reset();
    m_window.reset();
}
