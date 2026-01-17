#pragma once

#include <bitset>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// 键盘输入常量
namespace Key {
    constexpr int W = GLFW_KEY_W;
    constexpr int A = GLFW_KEY_A;
    constexpr int S = GLFW_KEY_S;
    constexpr int D = GLFW_KEY_D;
    constexpr int Q = GLFW_KEY_Q;
    constexpr int E = GLFW_KEY_E;
    constexpr int Escape = GLFW_KEY_ESCAPE;
    constexpr int Space = GLFW_KEY_SPACE;
    constexpr int LeftShift = GLFW_KEY_LEFT_SHIFT;
    constexpr int LeftControl = GLFW_KEY_LEFT_CONTROL;
    constexpr int Tab = GLFW_KEY_TAB;
    constexpr int Enter = GLFW_KEY_ENTER;
    constexpr int Up = GLFW_KEY_UP;
    constexpr int Down = GLFW_KEY_DOWN;
    constexpr int Left = GLFW_KEY_LEFT;
    constexpr int Right = GLFW_KEY_RIGHT;
}

// 鼠标按钮常量
namespace MouseButton {
    constexpr int Left = GLFW_MOUSE_BUTTON_LEFT;
    constexpr int Right = GLFW_MOUSE_BUTTON_RIGHT;
    constexpr int Middle = GLFW_MOUSE_BUTTON_MIDDLE;
}

class Inputs {
private:
    GLFWwindow* m_window;

    // 使用 std::bitset 来高效存储键/鼠标按钮的按下状态
    std::bitset<GLFW_KEY_LAST> m_currentKeys;
    std::bitset<GLFW_KEY_LAST> m_prevKeys;
    std::bitset<GLFW_MOUSE_BUTTON_LAST> m_currentMouseButtons;
    std::bitset<GLFW_MOUSE_BUTTON_LAST> m_prevMouseButtons;

    glm::vec2 m_mousePos{0.0f, 0.0f};
    glm::vec2 m_mouseDelta{0.0f, 0.0f};
    glm::vec2 m_lastMousePos{0.0f, 0.0f};
    bool m_isFirstMouse = true;

    // 鼠标捕获模式（用于 FPS 相机控制）
    bool m_cursorCaptured = false;
    bool m_wasLeftButtonPressed = false;  // 用于检测左键按下/释放

public:
    using Key = int;
    using MouseButton = int;

public:
    explicit Inputs(GLFWwindow* window);
    ~Inputs() = default;

    Inputs(const Inputs&) = delete;
    Inputs& operator=(const Inputs&) = delete;
    Inputs(Inputs&&) = delete;
    Inputs& operator=(Inputs&&) = delete;

    // 更新输入状态（每帧调用一次）
    void update();

    // 键盘状态查询
    bool isKeyPressed(Key key) const;
    bool isKeyJustPressed(Key key) const;
    bool isKeyJustReleased(Key key) const;

    // 鼠标按钮状态查询
    bool isMouseButtonPressed(MouseButton button) const;
    bool isMouseButtonJustPressed(MouseButton button) const;
    bool isMouseButtonJustReleased(MouseButton button) const;

    // 鼠标位置查询
    glm::vec2 getMousePosition() const;
    glm::vec2 getMouseDelta() const;

    // 鼠标模式控制
    void setCursorMode(int mode);
    bool isCursorCaptured() const { return m_cursorCaptured; }
    void setCursorCaptured(bool captured);

private:
    void updateMouseDelta();
    void updateCursorCapture();
};
