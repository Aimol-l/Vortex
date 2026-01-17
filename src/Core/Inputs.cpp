#include "Core/Inputs.h"
#include <GLFW/glfw3.h>

Inputs::Inputs(GLFWwindow* window)
    : m_window(window) {

    // 初始化鼠标位置
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    m_mousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    m_lastMousePos = m_mousePos;
}

void Inputs::update() {
    // 更新键位状态
    m_prevKeys = m_currentKeys;
    for (int i = 0; i < GLFW_KEY_LAST; ++i) {
        m_currentKeys[i] = (glfwGetKey(m_window, i) == GLFW_PRESS);
    }
    // 更新鼠标按钮状态
    m_prevMouseButtons = m_currentMouseButtons;
    for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; ++i) {
        m_currentMouseButtons[i] = (glfwGetMouseButton(m_window, i) == GLFW_PRESS);
    }
    // 更新鼠标捕获状态（左键控制）
    updateCursorCapture();
    // 更新鼠标位置和增量（只在捕获模式下计算增量）
    updateMouseDelta();
}

void Inputs::updateCursorCapture() {
    bool isLeftPressed = isMouseButtonPressed(MouseButton::Left);

    // 检测左键刚按下
    if (isLeftPressed && !m_wasLeftButtonPressed) {
        // 刚按下左键，捕获鼠标
        setCursorMode(GLFW_CURSOR_DISABLED);
        m_cursorCaptured = true;
        m_isFirstMouse = true;  // 重置，避免跳跃
    }
    // 检测左键刚释放
    else if (!isLeftPressed && m_wasLeftButtonPressed) {
        // 刚释放左键，恢复鼠标
        setCursorMode(GLFW_CURSOR_NORMAL);
        m_cursorCaptured = false;
        m_mouseDelta = glm::vec2(0.0f);  // 清零增量
    }

    m_wasLeftButtonPressed = isLeftPressed;
}

void Inputs::updateMouseDelta() {
    // 总是更新鼠标位置
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    glm::vec2 currentMousePos(static_cast<float>(xpos), static_cast<float>(ypos));

    m_mousePos = currentMousePos;

    // 只在捕获模式下计算增量
    if (m_cursorCaptured) {
        // 处理第一帧鼠标移动，防止大跳跃
        if (m_isFirstMouse) {
            m_lastMousePos = currentMousePos;
            m_isFirstMouse = false;
        }
        // m_mouseDelta = m_mousePos - m_lastMousePos;
        m_mouseDelta = m_lastMousePos - m_mousePos;  // 反转鼠标移动方向
        m_lastMousePos = m_mousePos;
    } else {
        // 未捕获时，增量为零
        m_mouseDelta = glm::vec2(0.0f);
    }
}

bool Inputs::isKeyPressed(Key key) const {
    if (key < 0 || key >= GLFW_KEY_LAST) return false;
    return m_currentKeys[key];
}

bool Inputs::isKeyJustPressed(Key key) const {
    if (key < 0 || key >= GLFW_KEY_LAST) return false;
    return m_currentKeys[key] && !m_prevKeys[key];
}

bool Inputs::isKeyJustReleased(Key key) const {
    if (key < 0 || key >= GLFW_KEY_LAST) return false;
    return !m_currentKeys[key] && m_prevKeys[key];
}

bool Inputs::isMouseButtonPressed(MouseButton button) const {
    if (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST) return false;
    return m_currentMouseButtons[button];
}

bool Inputs::isMouseButtonJustPressed(MouseButton button) const {
    if (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST) return false;
    return m_currentMouseButtons[button] && !m_prevMouseButtons[button];
}

bool Inputs::isMouseButtonJustReleased(MouseButton button) const {
    if (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST) return false;
    return !m_currentMouseButtons[button] && m_prevMouseButtons[button];
}

glm::vec2 Inputs::getMousePosition() const {
    return m_mousePos;
}

glm::vec2 Inputs::getMouseDelta() const {
    return m_mouseDelta;
}

void Inputs::setCursorMode(int mode) {
    glfwSetInputMode(m_window, GLFW_CURSOR, mode);
}

void Inputs::setCursorCaptured(bool captured) {
    m_cursorCaptured = captured;
    if (captured) {
        setCursorMode(GLFW_CURSOR_DISABLED);
        m_isFirstMouse = true;
    } else {
        setCursorMode(GLFW_CURSOR_NORMAL);
        m_mouseDelta = glm::vec2(0.0f);
    }
}
