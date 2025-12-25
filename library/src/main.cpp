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
Camera camera(glm::vec3(0.0f, 1.5f, 8.0f));  // 调整相机初始位置，使其能更好地观察图书馆
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

// 辅助函数：使用PBR材质渲染模型
void renderModelWithPBRMaterial(Shader& pbrShader, Model& model, PBRTextureMaterial& mat, 
                                 glm::mat4& modelMatrix, bool useGloss = false) {
    pbrShader.use();
    pbrShader.setMat4("model", modelMatrix);
    
    // 设置是否使用GLOSS贴图（需要反转roughness）
    pbrShader.setBool("useGlossMap", useGloss);
    
    // 绑定材质纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mat.albedoTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mat.normalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mat.metallicTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mat.roughnessTex);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mat.aoTex);
    
    // 材质参数（真实值来自贴图，这些是乘子或默认值）
    pbrShader.setVec3("material.albedo", glm::vec3(1.0f));
    pbrShader.setFloat("material.metallic", 0.0f);
    pbrShader.setFloat("material.roughness", 0.6f);
    pbrShader.setFloat("material.ao", 1.0f);
    
    model.Draw(pbrShader);
}

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

    // ========= 加载所有模型 =========
    Model bookshelf("models/bookshelf.obj");
    Model libraryTable("models/library_table.obj");
    Model stool("models/stool.obj");
    Model waterDispenser("models/water_dispenser.obj");
    Model cube("models/cube.obj");
    Model sphere("models/sphere.obj");

    // ========= 加载所有 PBR 材质 =========
    PBRTextureMaterial oakMat = LoadMaterial_WoodVeneerOak_7760();        // 橡木（用于书架、桌子）
    PBRTextureMaterial woodFloorMat = LoadMaterial_WoodFloorAsh_4186();   // 木地板（用于地板）
    PBRTextureMaterial metalMat = LoadMaterial_MetalGalvanizedZinc_7184(); // 金属（用于饮水机）
    PBRTextureMaterial paintedMetalMat = LoadMaterial_MetalPaintedMatte_7037(); // 喷漆金属
    PBRTextureMaterial leatherMat = LoadMaterial_FabricLeatherCowhide_001(); // 皮革（用于椅子）
    PBRTextureMaterial tileMat = LoadMaterial_TilesTravertine_001();      // 大理石（用于墙面装饰）

    // PBR 着色器
    Shader pbrShader("shaders/pbr.vert", "shaders/pbr.frag");

    // ========= 设置 PBR 光照系统（6个点光源营造图书馆氛围）=========
    pbrShader.use();
    pbrShader.setInt("lightCount", 6);
    
    // 主灯光（中央上方，暖白色）
    pbrShader.setVec3("lights[0].position", glm::vec3(0.0f, 4.5f, 0.0f));
    pbrShader.setVec3("lights[0].color", glm::vec3(1.0f, 0.95f, 0.85f));
    pbrShader.setFloat("lights[0].intensity", 50.0f);
    
    // 左侧灯光（书架区域）
    pbrShader.setVec3("lights[1].position", glm::vec3(-5.0f, 4.0f, 0.0f));
    pbrShader.setVec3("lights[1].color", glm::vec3(1.0f, 0.98f, 0.9f));
    pbrShader.setFloat("lights[1].intensity", 40.0f);
    
    // 右侧灯光（书架区域）
    pbrShader.setVec3("lights[2].position", glm::vec3(5.0f, 4.0f, 0.0f));
    pbrShader.setVec3("lights[2].color", glm::vec3(1.0f, 0.98f, 0.9f));
    pbrShader.setFloat("lights[2].intensity", 40.0f);
    
    // 前方灯光（阅读区）
    pbrShader.setVec3("lights[3].position", glm::vec3(0.0f, 4.0f, -4.0f));
    pbrShader.setVec3("lights[3].color", glm::vec3(0.95f, 0.98f, 1.0f));
    pbrShader.setFloat("lights[3].intensity", 35.0f);
    
    // 后方灯光
    pbrShader.setVec3("lights[4].position", glm::vec3(0.0f, 4.0f, 4.0f));
    pbrShader.setVec3("lights[4].color", glm::vec3(0.9f, 0.95f, 1.0f));
    pbrShader.setFloat("lights[4].intensity", 30.0f);
    
    // 角落灯光（饮水机区域，稍冷色调）
    pbrShader.setVec3("lights[5].position", glm::vec3(6.0f, 3.5f, 6.0f));
    pbrShader.setVec3("lights[5].color", glm::vec3(0.85f, 0.9f, 1.0f));
    pbrShader.setFloat("lights[5].intensity", 25.0f);

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

        // 更新 PBR shader 的矩阵和相机
        pbrShader.use();
        pbrShader.setMat4("view", view);
        pbrShader.setMat4("projection", projection);
        pbrShader.setVec3("camPos", camera.Position);

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);  // 深色背景，营造图书馆氛围
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ========= 渲染地板 =========
        glm::mat4 floorMatrix = glm::mat4(1.0f);
        floorMatrix = glm::translate(floorMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        floorMatrix = glm::scale(floorMatrix, glm::vec3(15.0f, 0.1f, 15.0f));  // 大尺寸地板
        renderModelWithPBRMaterial(pbrShader, cube, woodFloorMat, floorMatrix);

        // ========= 渲染书架（左侧墙，多个书架）=========
        // 左侧书架 1
        glm::mat4 bookshelf1Matrix = glm::mat4(1.0f);
        bookshelf1Matrix = glm::translate(bookshelf1Matrix, glm::vec3(-6.0f, 1.0f, -3.0f));
        bookshelf1Matrix = glm::rotate(bookshelf1Matrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        bookshelf1Matrix = glm::scale(bookshelf1Matrix, glm::vec3(0.8f));
        renderModelWithPBRMaterial(pbrShader, bookshelf, oakMat, bookshelf1Matrix);

        // 左侧书架 2
        glm::mat4 bookshelf2Matrix = glm::mat4(1.0f);
        bookshelf2Matrix = glm::translate(bookshelf2Matrix, glm::vec3(-6.0f, 1.0f, 0.0f));
        bookshelf2Matrix = glm::rotate(bookshelf2Matrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        bookshelf2Matrix = glm::scale(bookshelf2Matrix, glm::vec3(0.8f));
        renderModelWithPBRMaterial(pbrShader, bookshelf, oakMat, bookshelf2Matrix);

        // 左侧书架 3
        glm::mat4 bookshelf3Matrix = glm::mat4(1.0f);
        bookshelf3Matrix = glm::translate(bookshelf3Matrix, glm::vec3(-6.0f, 1.0f, 3.0f));
        bookshelf3Matrix = glm::rotate(bookshelf3Matrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        bookshelf3Matrix = glm::scale(bookshelf3Matrix, glm::vec3(0.8f));
        renderModelWithPBRMaterial(pbrShader, bookshelf, oakMat, bookshelf3Matrix);

        // ========= 渲染书架（右侧墙）=========
        // 右侧书架 1
        glm::mat4 bookshelf4Matrix = glm::mat4(1.0f);
        bookshelf4Matrix = glm::translate(bookshelf4Matrix, glm::vec3(6.0f, 1.0f, -3.0f));
        bookshelf4Matrix = glm::rotate(bookshelf4Matrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        bookshelf4Matrix = glm::scale(bookshelf4Matrix, glm::vec3(0.8f));
        renderModelWithPBRMaterial(pbrShader, bookshelf, oakMat, bookshelf4Matrix);

        // 右侧书架 2
        glm::mat4 bookshelf5Matrix = glm::mat4(1.0f);
        bookshelf5Matrix = glm::translate(bookshelf5Matrix, glm::vec3(6.0f, 1.0f, 0.0f));
        bookshelf5Matrix = glm::rotate(bookshelf5Matrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        bookshelf5Matrix = glm::scale(bookshelf5Matrix, glm::vec3(0.8f));
        renderModelWithPBRMaterial(pbrShader, bookshelf, oakMat, bookshelf5Matrix);

        // 右侧书架 3
        glm::mat4 bookshelf6Matrix = glm::mat4(1.0f);
        bookshelf6Matrix = glm::translate(bookshelf6Matrix, glm::vec3(6.0f, 1.0f, 3.0f));
        bookshelf6Matrix = glm::rotate(bookshelf6Matrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        bookshelf6Matrix = glm::scale(bookshelf6Matrix, glm::vec3(0.8f));
        renderModelWithPBRMaterial(pbrShader, bookshelf, oakMat, bookshelf6Matrix);

        // ========= 渲染中央桌子 =========
        glm::mat4 tableMatrix = glm::mat4(1.0f);
        tableMatrix = glm::translate(tableMatrix, glm::vec3(0.0f, 0.0f, -1.0f));
        tableMatrix = glm::scale(tableMatrix, glm::vec3(1.2f));
        renderModelWithPBRMaterial(pbrShader, libraryTable, oakMat, tableMatrix);

        // ========= 渲染椅子（桌子周围）=========
        // 椅子 1（桌子左侧）
        glm::mat4 stool1Matrix = glm::mat4(1.0f);
        stool1Matrix = glm::translate(stool1Matrix, glm::vec3(-2.0f, 0.0f, -1.0f));
        stool1Matrix = glm::scale(stool1Matrix, glm::vec3(1.0f));
        renderModelWithPBRMaterial(pbrShader, stool, leatherMat, stool1Matrix, true);  // 使用GLOSS贴图

        // 椅子 2（桌子右侧）
        glm::mat4 stool2Matrix = glm::mat4(1.0f);
        stool2Matrix = glm::translate(stool2Matrix, glm::vec3(2.0f, 0.0f, -1.0f));
        stool2Matrix = glm::scale(stool2Matrix, glm::vec3(1.0f));
        renderModelWithPBRMaterial(pbrShader, stool, leatherMat, stool2Matrix, true);  // 使用GLOSS贴图

        // 椅子 3（桌子前方）
        glm::mat4 stool3Matrix = glm::mat4(1.0f);
        stool3Matrix = glm::translate(stool3Matrix, glm::vec3(0.0f, 0.0f, -3.5f));
        stool3Matrix = glm::rotate(stool3Matrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        stool3Matrix = glm::scale(stool3Matrix, glm::vec3(1.0f));
        renderModelWithPBRMaterial(pbrShader, stool, leatherMat, stool3Matrix, true);  // 使用GLOSS贴图

        // 椅子 4（桌子后方）
        glm::mat4 stool4Matrix = glm::mat4(1.0f);
        stool4Matrix = glm::translate(stool4Matrix, glm::vec3(0.0f, 0.0f, 1.5f));
        stool4Matrix = glm::scale(stool4Matrix, glm::vec3(1.0f));
        renderModelWithPBRMaterial(pbrShader, stool, leatherMat, stool4Matrix, true);  // 使用GLOSS贴图

        // ========= 渲染饮水机（角落）=========
        glm::mat4 dispenserMatrix = glm::mat4(1.0f);
        dispenserMatrix = glm::translate(dispenserMatrix, glm::vec3(5.5f, 0.0f, 5.5f));
        dispenserMatrix = glm::rotate(dispenserMatrix, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        dispenserMatrix = glm::scale(dispenserMatrix, glm::vec3(1.0f));
        renderModelWithPBRMaterial(pbrShader, waterDispenser, metalMat, dispenserMatrix);

        // ========= 渲染装饰球体（桌面装饰）=========
        glm::mat4 sphereMatrix = glm::mat4(1.0f);
        sphereMatrix = glm::translate(sphereMatrix, glm::vec3(0.0f, 0.8f, -1.0f));
        sphereMatrix = glm::scale(sphereMatrix, glm::vec3(0.3f));
        renderModelWithPBRMaterial(pbrShader, sphere, paintedMetalMat, sphereMatrix);

        // ========= 渲染墙面装饰（使用cube作为装饰块）=========
        // 左侧墙装饰
        glm::mat4 wallDecor1Matrix = glm::mat4(1.0f);
        wallDecor1Matrix = glm::translate(wallDecor1Matrix, glm::vec3(-6.5f, 2.5f, 0.0f));
        wallDecor1Matrix = glm::scale(wallDecor1Matrix, glm::vec3(0.2f, 2.0f, 8.0f));
        renderModelWithPBRMaterial(pbrShader, cube, tileMat, wallDecor1Matrix, true);  // 使用GLOSS贴图

        // 右侧墙装饰
        glm::mat4 wallDecor2Matrix = glm::mat4(1.0f);
        wallDecor2Matrix = glm::translate(wallDecor2Matrix, glm::vec3(6.5f, 2.5f, 0.0f));
        wallDecor2Matrix = glm::scale(wallDecor2Matrix, glm::vec3(0.2f, 2.0f, 8.0f));
        renderModelWithPBRMaterial(pbrShader, cube, tileMat, wallDecor2Matrix, true);  // 使用GLOSS贴图

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
