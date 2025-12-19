#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <windows.h>  // 用于设置控制台编码（解决乱码）

#include "Shader.h"
#include "Model.h"
#include "Camera.h"
#include "Texture.h"

// 相机相关全局变量
Camera camera(glm::vec3(0.0f, 0.5f, 5.0f));
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int windowWidth = 800;
int windowHeight = 600;

// 鼠标捕获状态：true 表示用于视角控制（锁定到窗口中间），false 表示释放鼠标操作 UI
bool mouseCaptured = true;
bool ctrlWasPressed = false; // 用于检测 Ctrl 的“按下边缘”

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
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "First Person Camera - Library Scene", nullptr, nullptr);
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

    // 使用相对路径（相对于可执行文件所在目录）
    // 注意：运行时需要确保 shaders/ 和 models/ 文件夹与 .exe 在同一目录
    Model model2("models/cube.obj");
    Model model1("models/geodesic_classI_2.obj");

    // Blinn-Phong 着色器（继续用于 model2）
    Shader phongShader("shaders/basic.vert", "shaders/basic.frag");

    // PBR 着色器（用于 model1）
    Shader pbrShader("shaders/pbr.vert", "shaders/pbr.frag");

    // 加载 WoodVeneerOak PBR 纹理材质
    PBRTextureMaterial oakMat = LoadMaterial_WoodVeneerOak_7760();

    // ========= Blinn-Phong 光照参数（只给 model2 用）=========
    phongShader.use();
    phongShader.setVec3("material.diffuse", glm::vec3(0.8f));    // 漫反射：浅灰
    phongShader.setVec3("material.specular", glm::vec3(1.0f));   // 镜面反射：白色
    phongShader.setFloat("material.shininess", 64.0f);           // 高光锐利度

    phongShader.setVec3("dirLights[0].direction", glm::vec3(0.5f, -1.0f, -0.5f));  // 斜右下
    phongShader.setVec3("dirLights[0].color", glm::vec3(1.0f, 1.0f, 0.8f));        // 暖白色
    phongShader.setFloat("dirLights[0].intensity", 1.2f);

    phongShader.setVec3("dirLights[1].direction", glm::vec3(-0.5f, -1.0f, 0.5f));   // 斜左下
    phongShader.setVec3("dirLights[1].color", glm::vec3(0.8f, 1.0f, 1.0f));        // 冷白色
    phongShader.setFloat("dirLights[1].intensity", 1.0f);

    // ========= PBR 光照参数（简单两个点光源）=========
    pbrShader.use();
    // 相机位置先设一次（每帧会更新）
    pbrShader.setInt("lightCount", 2);
    // 点光 1
    pbrShader.setVec3("lights[0].position", glm::vec3(2.0f, 4.0f, 2.0f));
    pbrShader.setVec3("lights[0].color", glm::vec3(1.0f, 0.95f, 0.8f));
    pbrShader.setFloat("lights[0].intensity", 40.0f);
    // 点光 2
    pbrShader.setVec3("lights[1].position", glm::vec3(-2.0f, 4.0f, -1.5f));
    pbrShader.setVec3("lights[1].color", glm::vec3(0.8f, 0.9f, 1.0f));
    pbrShader.setFloat("lights[1].intensity", 30.0f);

    // 绑定采样器编号（纹理单元）
    pbrShader.setInt("albedoMap",    0);
    pbrShader.setInt("normalMap",    1);
    pbrShader.setInt("metallicMap",  2);
    pbrShader.setInt("roughnessMap", 3);
    pbrShader.setInt("aoMap",        4);

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
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)windowWidth / (float)windowHeight,
            0.1f,
            100.0f
        );

        // 更新 Blinn-Phong shader 的矩阵和相机
        phongShader.use();
        phongShader.setMat4("view", view);
        phongShader.setMat4("projection", projection);
        phongShader.setVec3("viewPos", camera.Position);

        // 更新 PBR shader 的矩阵和相机
        pbrShader.use();
        pbrShader.setMat4("view", view);
        pbrShader.setMat4("projection", projection);
        pbrShader.setVec3("camPos", camera.Position);

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ========= 使用 PBR 渲染 model1（左边）=========
        pbrShader.use();

        // 绑定 WoodVeneerOak 纹理到各个纹理单元
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, oakMat.albedoTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, oakMat.normalTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, oakMat.metallicTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, oakMat.roughnessTex);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, oakMat.aoTex);

        // 暂时用一个“中等粗糙、非金属、AO=1”的默认数值做兜底
        pbrShader.setVec3("material.albedo", glm::vec3(1.0f)); // 真实颜色来自贴图，这个只是乘子
        pbrShader.setFloat("material.metallic", 0.0f);
        pbrShader.setFloat("material.roughness", 0.6f);
        pbrShader.setFloat("material.ao", 1.0f);

        glm::mat4 modelMat1 = glm::mat4(1.0f);
        modelMat1 = glm::translate(modelMat1, glm::vec3(-1.2f, 0.0f, 0.0f));
        modelMat1 = glm::scale(modelMat1, glm::vec3(0.8f));
        pbrShader.setMat4("model", modelMat1);
        model1.Draw(pbrShader);

        // ========= 使用原 Blinn-Phong 渲染 model2（右边）=========
        phongShader.use();
        glm::mat4 modelMat2 = glm::mat4(1.0f);
        modelMat2 = glm::translate(modelMat2, glm::vec3(0.8f, 0.0f, 0.0f));
        phongShader.setMat4("model", modelMat2);
        model2.Draw(phongShader);

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