#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <windows.h>  // 用于设置控制台编码（解决乱码）

// ImGui 集成
// 注意：需要先下载ImGui到external/imgui目录
// 下载地址：https://github.com/ocornut/imgui/releases
// 或使用 git clone https://github.com/ocornut/imgui.git external/imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Shader.h"
#include "Camera.h"
#include "Scene.h"
#include "ShadowManager.h"

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

    // ========= 初始化 ImGui =========
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // 启用键盘导航
    
    // 设置ImGui样式
    ImGui::StyleColorsDark();
    
    // 初始化ImGui平台/渲染器绑定
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ========= 初始化场景 =========
    Scene scene;
    scene.Initialize();

    // PBR 着色器
    Shader pbrShader("shaders/pbr.vert", "shaders/pbr.frag");

    // ========= 初始化阴影管理器 =========
    ShadowManager shadowManager;
    shadowManager.Initialize(2048);  // 2048x2048阴影贴图

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

        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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

        // 根据时间设置背景颜色
        glm::vec4 bgColor = scene.CalculateBackgroundColor(static_cast<float>(scene.GetTime()));
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ========= ImGui UI：时间滑动条（右上角）=========
        {
            // 设置窗口位置和大小（右上角）
            ImGui::SetNextWindowPos(ImVec2(fbW - 180, 10), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(170, 50), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));  // 透明背景
            ImGui::Begin("##TimeControl", nullptr, 
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoBackground);
            
            static float timeValue = 12.0f;  // 默认12点
            
            // 计算时间（HH:MM格式）
            int hours = static_cast<int>(timeValue);
            int minutes = static_cast<int>((timeValue - hours) * 60.0f);
            
            // 创建时间字符串
            char timeStr[6];
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);
            
            // 显示时间文本（居中）
            float textWidth = ImGui::CalcTextSize(timeStr).x;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
            ImGui::Text("%s", timeStr);
            
            // 无标签的滑动条
            ImGui::PushItemWidth(150.0f);
            ImGui::SliderFloat("##TimeSlider", &timeValue, 0.0f, 24.0f, "");
            ImGui::PopItemWidth();
            
            // 更新场景时间（每次帧都更新，确保同步）
            scene.SetTime(timeValue);
            
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::End();
        }
        
        // 根据时间设置光照（每帧更新，因为时间可能改变）
        scene.SetupLighting(pbrShader);

        // ========= 第一步：渲染阴影贴图（从光源视角）=========
        scene.RenderShadowMap(shadowManager);

        // ========= 第二步：恢复视口 =========
        glViewport(0, 0, windowWidth, windowHeight);

        // ========= 第三步：设置阴影相关uniform =========
        scene.SetupShadowUniforms(pbrShader, shadowManager);

        // ========= 第四步：渲染主场景（应用阴影）=========
        scene.Render(pbrShader, view, projection, camera.Position);

        // 渲染ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // 清理阴影管理器
    shadowManager.Cleanup();

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
