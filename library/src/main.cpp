#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <windows.h>  // 用于设置控制台编码（解决乱码）

#include "Shader.h"
#include "Camera.h"
#include "Scene.h"

// 相机相关全局变量
Camera camera(glm::vec3(0.0f, 1.5f, 4.0f));  // 调整相机初始位置，使其能更好地观察图书馆
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int windowWidth = 800;
int windowHeight = 600;

// 鼠标捕获状态：true 表示用于视角控制（锁定到窗口中间），false 表示释放鼠标操作 UI
bool mouseCaptured = true;
bool ctrlWasPressed = false; // 用于检测 Ctrl 的"按下边缘"

// 回调函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);


int main() {
    // ========= 解决控制台乱码（Windows专属） =========
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    windowWidth = 800;
    windowHeight = 600;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Library Scene - First Person Camera", nullptr, nullptr);
    if (!window) {
        std::cerr << "GLFW窗口创建失败" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // 设置回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // 初始：隐藏光标并捕获鼠标输入，用于第一人称视角
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD初始化失败" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ========= 初始化场景 =========
    Scene scene;
    scene.Initialize();

    // PBR 着色器
    Shader pbrShader("shaders/pbr.vert", "shaders/pbr.frag");

    // 设置光照
    scene.SetupLighting(pbrShader);

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        // 计算帧时间
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理输入
        processInput(window);

        // 更新视图和投影矩阵
        glm::mat4 view = camera.GetViewMatrix();

        
        int fbW = windowWidth;
        int fbH = windowHeight;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbH <= 0) fbH = 1;
        if (fbW <= 0) fbW = 1;
        windowWidth = fbW;
        windowHeight = fbH;

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            static_cast<float>(fbW) / static_cast<float>(fbH),
            0.1f,
            100.0f
        );

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // 天蓝色背景，模拟窗外天空
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ========= 渲染场景 =========
        scene.Render(pbrShader, view, projection, camera.Position);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// 窗口大小回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

// 鼠标移动回调
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // 如果当前未捕获鼠标（用于操作 UI），则不更新相机
    if (!mouseCaptured) {
        // 让下次重新捕获时重新初始化 lastX/lastY
        firstMouse = true;
        return;
    }

    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // 注意这里是相反的，因为y坐标是从底部往顶部递增的

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// 鼠标滚轮回调
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// 输入处理
void processInput(GLFWwindow* window) {
    // ESC 退出
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Ctrl 切换鼠标捕获状态（按下一次切换一次）
    bool ctrlDown = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ||
                    (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
    if (ctrlDown && !ctrlWasPressed) {
        mouseCaptured = !mouseCaptured;

        if (mouseCaptured) {
            // 重新捕获鼠标：隐藏光标并锁定
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // 重置 firstMouse，避免切回时视角突然跳变
            firstMouse = true;
        } else {
            // 释放鼠标：显示光标
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    ctrlWasPressed = ctrlDown;

    // 只有在捕获鼠标时才响应 WASD（第一人称模式）
    if (mouseCaptured) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(0, deltaTime); // 前
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(1, deltaTime); // 后
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(2, deltaTime); // 左
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(3, deltaTime); // 右
    }
}
