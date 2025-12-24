#define NOMINMAX  // 防止Windows.h定义min/max宏，避免与std::min冲突
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>  // 用于std::min
#include <windows.h>  // 用于设置控制台编码（解决乱码）

#include "Shader.h"
#include "Model.h"
#include "Camera.h"
#include "Texture.h"

// 相机相关全局变量
Camera camera(glm::vec3(0.0f, 1.5f, 12.0f));  // 调整相机初始位置，适应扩大后的场景
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
                                 glm::mat4& modelMatrix, bool useGloss = false, bool useTriplanar = true) {
    pbrShader.use();
    pbrShader.setMat4("model", modelMatrix);
    
    // 设置是否使用GLOSS贴图（需要反转roughness）
    pbrShader.setBool("useGlossMap", useGloss);
    
    // 设置是否使用triplanar mapping
    pbrShader.setBool("useTriplanarMapping", useTriplanar);
    
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
    
    // 纯PBR渲染，不使用MTL颜色调制
    // 设置materialAlbedo为白色(1,1,1)，不进行任何调制
    pbrShader.setVec3("materialAlbedo", glm::vec3(1.0f, 1.0f, 1.0f));
    
    model.Draw(pbrShader);
}

// 辅助函数：渲染模型到阴影贴图（仅深度）
void renderModelToShadow(Shader& shadowShader, Model& model, glm::mat4& modelMatrix) {
    shadowShader.use();
    shadowShader.setMat4("model", modelMatrix);
    model.Draw(shadowShader);
}

// 辅助函数：使用MTL材质渲染模型（不使用PBR贴图，仅使用MTL颜色）
void renderModelWithMTLMaterial(Shader& pbrShader, Model& model, glm::mat4& modelMatrix) {
    pbrShader.use();
    pbrShader.setMat4("model", modelMatrix);
    
    // 禁用triplanar mapping，使用传统UV
    pbrShader.setBool("useTriplanarMapping", false);
    pbrShader.setBool("useGlossMap", false);
    
    // 创建简单的单色纹理或使用默认纹理
    // 这里我们使用一个简单的白色纹理作为占位符
    static GLuint whiteTexture = 0;
    if (whiteTexture == 0) {
        // 创建1x1白色纹理
        unsigned char whiteData[3] = {255, 255, 255};
        glGenTextures(1, &whiteTexture);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whiteData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    // 绑定白色纹理到所有纹理单元
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    
    // 纯PBR渲染，不使用MTL颜色
    // 设置materialAlbedo为白色(1,1,1)，不进行任何调制
    pbrShader.setVec3("materialAlbedo", glm::vec3(1.0f, 1.0f, 1.0f));
    
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
    
    // 阴影映射着色器
    Shader shadowShader("shaders/shadow.vert", "shaders/shadow.frag");

    // ========= 创建阴影贴图（为4个点光源各创建一个）=========
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    unsigned int shadowMapFBO[4];
    unsigned int shadowMap[4];
    
    for (int i = 0; i < 4; i++) {
        // 创建FBO
        glGenFramebuffers(1, &shadowMapFBO[i]);
        
        // 创建深度纹理
        glGenTextures(1, &shadowMap[i]);
        glBindTexture(GL_TEXTURE_2D, shadowMap[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                     SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        
        // 绑定FBO
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "阴影贴图FBO创建失败！" << std::endl;
        }
    }
    
    // ========= 创建方向光阴影贴图 =========
    unsigned int dirLightShadowMapFBO;
    unsigned int dirLightShadowMap;
    glGenFramebuffers(1, &dirLightShadowMapFBO);
    glGenTextures(1, &dirLightShadowMap);
    glBindTexture(GL_TEXTURE_2D, dirLightShadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float dirBorderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, dirBorderColor);
    
    glBindFramebuffer(GL_FRAMEBUFFER, dirLightShadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dirLightShadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "方向光阴影贴图FBO创建失败！" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ========= 设置 PBR 光照系统（顶灯光源将在主循环中动态设置）=========
    pbrShader.use();
    pbrShader.setInt("lightCount", 4);

    // 绑定采样器编号（纹理单元）
    pbrShader.setInt("albedoMap",    0);
    pbrShader.setInt("normalMap",    1);
    pbrShader.setInt("metallicMap",  2);
    pbrShader.setInt("roughnessMap", 3);
    pbrShader.setInt("aoMap",        4);
    
    // 绑定阴影贴图到纹理单元（从5开始）
    for (int i = 0; i < 4; i++) {
        char shadowMapName[32];
        snprintf(shadowMapName, sizeof(shadowMapName), "shadowMap[%d]", i);
        pbrShader.setInt(shadowMapName, 5 + i);
    }
    
    // 绑定方向光阴影贴图到纹理单元9
    pbrShader.setInt("dirLightShadowMap", 9);

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        // 计算帧时间
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理输入
        processInput(window);

        // ========= 第一步：渲染阴影贴图（从每个光源的视角）=========
        // 计算光源位置和光源空间矩阵
        int lightCountX = 2;  // 2列
        int lightCountZ = 2;  // 2行（2x2排布，共4个灯）
        float lightSpacingX = 20.0f / (lightCountX + 1);
        float lightSpacingZ = 30.0f / (lightCountZ + 1);
        
        glm::vec3 lightPositions[4];
        glm::mat4 lightSpaceMatrices[4];
        
        // 阴影贴图的投影参数
        float shadowNear = 0.1f;
        float shadowFar = 30.0f;
        float shadowSize = 25.0f;  // 阴影贴图覆盖的范围
        
        int lightIndex = 0;
        for (int i = 0; i < lightCountX; i++) {
            for (int j = 0; j < lightCountZ; j++) {
                float xPos = -10.0f + (i + 1) * lightSpacingX;
                float zPos = -15.0f + (j + 1) * lightSpacingZ;
                lightPositions[lightIndex] = glm::vec3(xPos, 5.8f, zPos);
                
                // 计算光源空间矩阵（从光源向下看，正交投影）
                glm::mat4 lightProjection = glm::ortho(-shadowSize, shadowSize, -shadowSize, shadowSize, shadowNear, shadowFar);
                glm::mat4 lightView = glm::lookAt(
                    lightPositions[lightIndex],
                    lightPositions[lightIndex] + glm::vec3(0.0f, -1.0f, 0.0f),  // 向下看
                    glm::vec3(0.0f, 0.0f, 1.0f)  // 上方向
                );
                lightSpaceMatrices[lightIndex] = lightProjection * lightView;
                
                // 渲染到阴影贴图
                glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
                glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO[lightIndex]);
                glClear(GL_DEPTH_BUFFER_BIT);
                
                shadowShader.use();
                shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrices[lightIndex]);
                
                // 渲染所有需要投射阴影的物体（不包括灯本身和天花板）
                // 地板
                glm::mat4 floorMatrix = glm::mat4(1.0f);
                floorMatrix = glm::translate(floorMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
                floorMatrix = glm::scale(floorMatrix, glm::vec3(25.0f, 0.1f, 40.0f));
                renderModelToShadow(shadowShader, cube, floorMatrix);
                
                // 墙壁
                glm::mat4 leftWallMatrix = glm::mat4(1.0f);
                leftWallMatrix = glm::translate(leftWallMatrix, glm::vec3(-12.5f, 3.0f, 0.0f));
                leftWallMatrix = glm::scale(leftWallMatrix, glm::vec3(0.2f, 6.0f, 40.0f));
                renderModelToShadow(shadowShader, cube, leftWallMatrix);
                
                glm::mat4 rightWallMatrix = glm::mat4(1.0f);
                rightWallMatrix = glm::translate(rightWallMatrix, glm::vec3(12.5f, 3.0f, 0.0f));
                rightWallMatrix = glm::scale(rightWallMatrix, glm::vec3(0.2f, 6.0f, 40.0f));
                renderModelToShadow(shadowShader, cube, rightWallMatrix);
                
                glm::mat4 frontWallMatrix = glm::mat4(1.0f);
                frontWallMatrix = glm::translate(frontWallMatrix, glm::vec3(0.0f, 3.0f, -20.0f));
                frontWallMatrix = glm::scale(frontWallMatrix, glm::vec3(25.0f, 6.0f, 0.2f));
                renderModelToShadow(shadowShader, cube, frontWallMatrix);
                
                glm::mat4 backWallMatrix = glm::mat4(1.0f);
                backWallMatrix = glm::translate(backWallMatrix, glm::vec3(0.0f, 3.0f, 20.0f));
                backWallMatrix = glm::scale(backWallMatrix, glm::vec3(25.0f, 6.0f, 0.2f));
                renderModelToShadow(shadowShader, cube, backWallMatrix);
                
                // 书架、桌子、椅子、饮水机
                float tableRowZPositions[6] = {-15.0f, -9.0f, -3.0f, 3.0f, 9.0f, 15.0f};
                for (int row = 0; row < 6; row++) {
                    float zPos = tableRowZPositions[row];
                    
                    // 书架
                    glm::mat4 bookshelf1Matrix = glm::mat4(1.0f);
                    bookshelf1Matrix = glm::translate(bookshelf1Matrix, glm::vec3(-10.0f, 0.1f, zPos+0.1f));
                    bookshelf1Matrix = glm::scale(bookshelf1Matrix, glm::vec3(0.8f));
                    renderModelToShadow(shadowShader, bookshelf, bookshelf1Matrix);
                    
                    glm::mat4 bookshelf2Matrix = glm::mat4(1.0f);
                    bookshelf2Matrix = glm::translate(bookshelf2Matrix, glm::vec3(-10.0f, 0.1f, zPos-0.1f));
                    bookshelf2Matrix = glm::rotate(bookshelf2Matrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    bookshelf2Matrix = glm::scale(bookshelf2Matrix, glm::vec3(0.8f));
                    renderModelToShadow(shadowShader, bookshelf, bookshelf2Matrix);
                    
                    // 桌子
                    float singleTableShortSide = 0.8f * 1.2f;
                    for (int tableIdx = 0; tableIdx < 12; tableIdx++) {
                        float xOffset = (tableIdx - 5.5f) * singleTableShortSide;
                        glm::mat4 tableMatrix = glm::mat4(1.0f);
                        tableMatrix = glm::translate(tableMatrix, glm::vec3(xOffset, 0.0f, zPos));
                        tableMatrix = glm::scale(tableMatrix, glm::vec3(1.2f));
                        renderModelToShadow(shadowShader, libraryTable, tableMatrix);
                    }
                    
                    // 椅子
                    float totalTableLength = singleTableShortSide * 12.0f;
                    float totalTableWidth = 2.0f * 1.2f;
                    int chairsPerSide = 8;
                    float chairSpacing = totalTableLength / (chairsPerSide + 1);
                    
                    for (int chairIdx = 0; chairIdx < chairsPerSide; chairIdx++) {
                        float xPos_chair = -totalTableLength / 2.0f + (chairIdx + 1) * chairSpacing;
                        
                        glm::mat4 stoolMatrix1 = glm::mat4(1.0f);
                        stoolMatrix1 = glm::translate(stoolMatrix1, glm::vec3(xPos_chair, 0.0f, zPos - totalTableWidth / 2.0f - 0.4f));
                        stoolMatrix1 = glm::rotate(stoolMatrix1, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                        renderModelToShadow(shadowShader, stool, stoolMatrix1);
                        
                        glm::mat4 stoolMatrix2 = glm::mat4(1.0f);
                        stoolMatrix2 = glm::translate(stoolMatrix2, glm::vec3(xPos_chair, 0.0f, zPos + totalTableWidth / 2.0f + 0.4f));
                        renderModelToShadow(shadowShader, stool, stoolMatrix2);
                    }
                }
                
                // 饮水机
                glm::mat4 dispenserMatrix = glm::mat4(1.0f);
                dispenserMatrix = glm::translate(dispenserMatrix, glm::vec3(11.0f, 0.0f, 18.0f));
                dispenserMatrix = glm::rotate(dispenserMatrix, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                renderModelToShadow(shadowShader, waterDispenser, dispenserMatrix);
                
                lightIndex++;
            }
        }
        
        // ========= 渲染方向光阴影贴图（从右侧窗户方向照射）=========
        // 方向光从右侧（窗户方向）照射进来，方向是从右向左（负X方向）
        // 注意：在着色器中会取反，所以这里设置(1, 0.4, 0)，取反后变成(-1, -0.4, 0)，即从右向左斜向下照射
        // Y分量为正值，取反后变成负值，让光线向下倾斜，能更好地照亮桌面
        glm::vec3 dirLightDirection = glm::normalize(glm::vec3(1.0f, 0.4f, 0.0f));  // 从右侧斜向下照射
        glm::vec3 dirLightTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // 场景中心
        glm::vec3 dirLightPos = dirLightTarget - dirLightDirection * 20.0f;  // 光源位置（在场景外）
        
        // 计算方向光光源空间矩阵（正交投影，覆盖整个场景）
        float dirShadowSize = 30.0f;  // 方向光阴影贴图覆盖范围
        glm::mat4 dirLightProjection = glm::ortho(-dirShadowSize, dirShadowSize, -dirShadowSize, dirShadowSize, 0.1f, 50.0f);
        glm::mat4 dirLightView = glm::lookAt(dirLightPos, dirLightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 dirLightSpaceMatrix = dirLightProjection * dirLightView;
        
        {
            // 渲染到方向光阴影贴图
            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, dirLightShadowMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            
            shadowShader.use();
            shadowShader.setMat4("lightSpaceMatrix", dirLightSpaceMatrix);
            
            // 渲染所有需要投射阴影的物体（与点光源相同）
            // 地板
            glm::mat4 dirFloorMatrix = glm::mat4(1.0f);
            dirFloorMatrix = glm::translate(dirFloorMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
            dirFloorMatrix = glm::scale(dirFloorMatrix, glm::vec3(25.0f, 0.1f, 40.0f));
            renderModelToShadow(shadowShader, cube, dirFloorMatrix);
            
            // 墙壁（不包括右侧墙壁，因为要改为落地窗）
            glm::mat4 dirLeftWallMatrix = glm::mat4(1.0f);
            dirLeftWallMatrix = glm::translate(dirLeftWallMatrix, glm::vec3(-12.5f, 3.0f, 0.0f));
            dirLeftWallMatrix = glm::scale(dirLeftWallMatrix, glm::vec3(0.2f, 6.0f, 40.0f));
            renderModelToShadow(shadowShader, cube, dirLeftWallMatrix);
            
            // 注意：右侧墙壁不渲染到阴影贴图，因为要改为落地窗
            glm::mat4 dirFrontWallMatrix = glm::mat4(1.0f);
            dirFrontWallMatrix = glm::translate(dirFrontWallMatrix, glm::vec3(0.0f, 3.0f, -20.0f));
            dirFrontWallMatrix = glm::scale(dirFrontWallMatrix, glm::vec3(25.0f, 6.0f, 0.2f));
            renderModelToShadow(shadowShader, cube, dirFrontWallMatrix);
            
            glm::mat4 dirBackWallMatrix = glm::mat4(1.0f);
            dirBackWallMatrix = glm::translate(dirBackWallMatrix, glm::vec3(0.0f, 3.0f, 20.0f));
            dirBackWallMatrix = glm::scale(dirBackWallMatrix, glm::vec3(25.0f, 6.0f, 0.2f));
            renderModelToShadow(shadowShader, cube, dirBackWallMatrix);
            
            // 书架、桌子、椅子、饮水机（与点光源相同）
            float dirTableRowZPositions[6] = {-15.0f, -9.0f, -3.0f, 3.0f, 9.0f, 15.0f};
            for (int row = 0; row < 6; row++) {
                float zPos = dirTableRowZPositions[row];
                
                // 书架
                glm::mat4 dirBookshelf1Matrix = glm::mat4(1.0f);
                dirBookshelf1Matrix = glm::translate(dirBookshelf1Matrix, glm::vec3(-10.0f, 0.1f, zPos+0.5f));
                dirBookshelf1Matrix = glm::scale(dirBookshelf1Matrix, glm::vec3(0.8f));
                renderModelToShadow(shadowShader, bookshelf, dirBookshelf1Matrix);
                
                glm::mat4 dirBookshelf2Matrix = glm::mat4(1.0f);
                dirBookshelf2Matrix = glm::translate(dirBookshelf2Matrix, glm::vec3(-10.0f, 0.1f, zPos-0.5f));
                dirBookshelf2Matrix = glm::rotate(dirBookshelf2Matrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                dirBookshelf2Matrix = glm::scale(dirBookshelf2Matrix, glm::vec3(0.8f));
                renderModelToShadow(shadowShader, bookshelf, dirBookshelf2Matrix);
                
                // 桌子
                float dirSingleTableShortSide = 0.8f * 1.2f;
                for (int tableIdx = 0; tableIdx < 12; tableIdx++) {
                    float xOffset = (tableIdx - 5.5f) * dirSingleTableShortSide;
                    glm::mat4 dirTableMatrix = glm::mat4(1.0f);
                    dirTableMatrix = glm::translate(dirTableMatrix, glm::vec3(xOffset, 0.0f, zPos));
                    dirTableMatrix = glm::scale(dirTableMatrix, glm::vec3(1.2f));
                    renderModelToShadow(shadowShader, libraryTable, dirTableMatrix);
                }
                
                // 椅子
                float dirTotalTableLength = dirSingleTableShortSide * 12.0f;
                float dirTotalTableWidth = 2.0f * 1.2f;
                int dirChairsPerSide = 8;
                float dirChairSpacing = dirTotalTableLength / (dirChairsPerSide + 1);
                
                for (int chairIdx = 0; chairIdx < dirChairsPerSide; chairIdx++) {
                    float xPos_chair = -dirTotalTableLength / 2.0f + (chairIdx + 1) * dirChairSpacing;
                    
                    glm::mat4 dirStoolMatrix1 = glm::mat4(1.0f);
                    dirStoolMatrix1 = glm::translate(dirStoolMatrix1, glm::vec3(xPos_chair, 0.0f, zPos - dirTotalTableWidth / 2.0f - 0.4f));
                    dirStoolMatrix1 = glm::rotate(dirStoolMatrix1, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    renderModelToShadow(shadowShader, stool, dirStoolMatrix1);
                    
                    glm::mat4 dirStoolMatrix2 = glm::mat4(1.0f);
                    dirStoolMatrix2 = glm::translate(dirStoolMatrix2, glm::vec3(xPos_chair, 0.0f, zPos + dirTotalTableWidth / 2.0f + 0.4f));
                    renderModelToShadow(shadowShader, stool, dirStoolMatrix2);
                }
            }
            
            // 饮水机
            glm::mat4 dirDispenserMatrix = glm::mat4(1.0f);
            dirDispenserMatrix = glm::translate(dirDispenserMatrix, glm::vec3(11.0f, 0.0f, 18.0f));
            dirDispenserMatrix = glm::rotate(dirDispenserMatrix, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            renderModelToShadow(shadowShader, waterDispenser, dirDispenserMatrix);
        }
        
        // ========= 第二步：渲染主场景（应用阴影）=========
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, windowWidth, windowHeight);
        
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
        
        // 设置点光源空间矩阵
        for (int i = 0; i < 4; i++) {
            char lightSpaceName[32];
            snprintf(lightSpaceName, sizeof(lightSpaceName), "lightSpaceMatrix[%d]", i);
            pbrShader.setMat4(lightSpaceName, lightSpaceMatrices[i]);
        }
        
        // 设置方向光光源空间矩阵
        pbrShader.setMat4("dirLightSpaceMatrix", dirLightSpaceMatrix);
        
        // 绑定点光源阴影贴图到纹理单元
        for (int i = 0; i < 4; i++) {
            glActiveTexture(GL_TEXTURE5 + i);
            glBindTexture(GL_TEXTURE_2D, shadowMap[i]);
        }
        
        // 绑定方向光阴影贴图到纹理单元9
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, dirLightShadowMap);
        
        // 启用所有点光源的阴影
        for (int i = 0; i < 4; i++) {
            char shadowEnabledName[32];
            snprintf(shadowEnabledName, sizeof(shadowEnabledName), "shadowsEnabled[%d]", i);
            pbrShader.setBool(shadowEnabledName, true);
        }
        
        // 设置方向光参数（从右侧窗户方向照射进来）
        pbrShader.setBool("dirLightEnabled", true);
        pbrShader.setBool("dirLightShadowsEnabled", true);  // 启用方向光阴影
        pbrShader.setVec3("dirLight.direction", dirLightDirection);
        pbrShader.setVec3("dirLight.color", glm::vec3(1.0f, 0.98f, 0.95f));  // 更亮的阳光色
        pbrShader.setFloat("dirLight.intensity", 20.0f);  // 提高方向光强度

        glClearColor(0.7f, 0.85f, 0.95f, 1.0f);  // 天蓝色背景，偏白（模拟天空）
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ========= 渲染地板 =========
        glm::mat4 floorMatrix = glm::mat4(1.0f);
        floorMatrix = glm::translate(floorMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        floorMatrix = glm::scale(floorMatrix, glm::vec3(25.0f, 0.1f, 40.0f));  // 扩大后的地板尺寸
        renderModelWithPBRMaterial(pbrShader, cube, woodFloorMat, floorMatrix, false, false);  // 禁用triplanar，使用传统UV

        // ========= 渲染四面墙 =========
        // 左侧墙（使用triplanar mapping，与前墙后墙一致）
        glm::mat4 leftWallMatrix = glm::mat4(1.0f);
        leftWallMatrix = glm::translate(leftWallMatrix, glm::vec3(-12.5f, 3.0f, 0.0f));
        leftWallMatrix = glm::scale(leftWallMatrix, glm::vec3(0.2f, 6.0f, 40.0f));
        renderModelWithPBRMaterial(pbrShader, cube, tileMat, leftWallMatrix, true);  // 使用triplanar

        // ========= 右侧墙改为落地窗 =========
        // 创建窗口框架（使用细长的立方体作为窗框）
        // 上窗框
        glm::mat4 topWindowFrameMatrix = glm::mat4(1.0f);
        topWindowFrameMatrix = glm::translate(topWindowFrameMatrix, glm::vec3(12.5f, 5.8f, 0.0f));
        topWindowFrameMatrix = glm::scale(topWindowFrameMatrix, glm::vec3(0.15f, 0.2f, 40.0f));
        renderModelWithPBRMaterial(pbrShader, cube, metalMat, topWindowFrameMatrix, false, false);
        
        // 下窗框
        glm::mat4 bottomWindowFrameMatrix = glm::mat4(1.0f);
        bottomWindowFrameMatrix = glm::translate(bottomWindowFrameMatrix, glm::vec3(12.5f, 0.2f, 0.0f));
        bottomWindowFrameMatrix = glm::scale(bottomWindowFrameMatrix, glm::vec3(0.15f, 0.2f, 40.0f));
        renderModelWithPBRMaterial(pbrShader, cube, metalMat, bottomWindowFrameMatrix, false, false);
        
        // 左侧窗框
        glm::mat4 leftWindowFrameMatrix = glm::mat4(1.0f);
        leftWindowFrameMatrix = glm::translate(leftWindowFrameMatrix, glm::vec3(12.5f, 3.0f, -20.0f));
        leftWindowFrameMatrix = glm::scale(leftWindowFrameMatrix, glm::vec3(0.15f, 6.0f, 0.2f));
        renderModelWithPBRMaterial(pbrShader, cube, metalMat, leftWindowFrameMatrix, false, false);
        
        // 右侧窗框
        glm::mat4 rightWindowFrameMatrix = glm::mat4(1.0f);
        rightWindowFrameMatrix = glm::translate(rightWindowFrameMatrix, glm::vec3(12.5f, 3.0f, 20.0f));
        rightWindowFrameMatrix = glm::scale(rightWindowFrameMatrix, glm::vec3(0.15f, 6.0f, 0.2f));
        renderModelWithPBRMaterial(pbrShader, cube, metalMat, rightWindowFrameMatrix, false, false);
        
        // 中间垂直窗框（将窗户分成两部分）
        glm::mat4 middleWindowFrameMatrix = glm::mat4(1.0f);
        middleWindowFrameMatrix = glm::translate(middleWindowFrameMatrix, glm::vec3(12.5f, 3.0f, 0.0f));
        middleWindowFrameMatrix = glm::scale(middleWindowFrameMatrix, glm::vec3(0.15f, 6.0f, 0.2f));
        renderModelWithPBRMaterial(pbrShader, cube, metalMat, middleWindowFrameMatrix, false, false);
        
        // 注意：窗户本身不渲染（透明），自然光从右侧照射进来

        // 前墙
        glm::mat4 frontWallMatrix = glm::mat4(1.0f);
        frontWallMatrix = glm::translate(frontWallMatrix, glm::vec3(0.0f, 3.0f, -20.0f));
        frontWallMatrix = glm::scale(frontWallMatrix, glm::vec3(25.0f, 6.0f, 0.2f));
        renderModelWithPBRMaterial(pbrShader, cube, tileMat, frontWallMatrix, true);

        // 后墙
        glm::mat4 backWallMatrix = glm::mat4(1.0f);
        backWallMatrix = glm::translate(backWallMatrix, glm::vec3(0.0f, 3.0f, 20.0f));
        backWallMatrix = glm::scale(backWallMatrix, glm::vec3(25.0f, 6.0f, 0.2f));
        renderModelWithPBRMaterial(pbrShader, cube, tileMat, backWallMatrix, true);

        // ========= 定义6排桌子的Z位置（从-15到15，间隔6单位）=========
        float tableRowZPositions[6] = {-15.0f, -9.0f, -3.0f, 3.0f, 9.0f, 15.0f};

        // ========= 渲染6排书架（每排2个背靠背，垂直于墙壁）=========
        for (int row = 0; row < 6; row++) {
            float zPos = tableRowZPositions[row];
            // 书架放在桌子的一端（左侧），2个书架背靠背，垂直于墙壁（不转90度）
            // 第一个书架（面向桌子）
            glm::mat4 bookshelf1Matrix = glm::mat4(1.0f);
            bookshelf1Matrix = glm::translate(bookshelf1Matrix, glm::vec3(-10.0f, 0.1f, zPos+0.1f));  // Y=0.0f使其贴地
            bookshelf1Matrix = glm::scale(bookshelf1Matrix, glm::vec3(0.8f));
            // 使用PBR材质渲染
            renderModelWithPBRMaterial(pbrShader, bookshelf, oakMat, bookshelf1Matrix);

            // 第二个书架（背靠背，面向墙）
            glm::mat4 bookshelf2Matrix = glm::mat4(1.0f);
            bookshelf2Matrix = glm::translate(bookshelf2Matrix, glm::vec3(-10.0f, 0.1f, zPos-0.1f));  // Y=0.0f使其贴地
            bookshelf2Matrix = glm::rotate(bookshelf2Matrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // 转180度使其背靠背
            bookshelf2Matrix = glm::scale(bookshelf2Matrix, glm::vec3(0.8f));
            // 使用PBR材质渲染
            renderModelWithPBRMaterial(pbrShader, bookshelf, oakMat, bookshelf2Matrix);
        }

        // ========= 渲染6排桌子（每排12张桌子短边相连，进一步加长）=========
        // 桌子尺寸：原始模型约2单位宽（X方向），0.8单位长（Z方向），缩放1.2后
        // 12张桌子短边相连（沿X轴排列，不旋转）：总宽度约 0.96 * 12 = 11.52单位（X方向），长度2.4单位（Z方向）
        // 组合后的长边是11.52单位（X方向），椅子应该放在X方向的两侧（左右两侧）
        
        float singleTableShortSide = 0.8f * 1.2f;  // 单张桌子短边（沿Z轴）
        float singleTableLongSide = 2.0f * 1.2f;   // 单张桌子长边（沿X轴）
        float totalTableLength = singleTableShortSide * 12.0f; // 12张桌子总长度（X方向，组合后的长边）
        float totalTableWidth = singleTableLongSide; // 组合后短边（Z方向）
        
        for (int row = 0; row < 6; row++) {
            float zPos = tableRowZPositions[row];
            
            // 渲染12张桌子，短边相连（沿X轴排列，不旋转）
            for (int tableIdx = 0; tableIdx < 12; tableIdx++) {
                float xOffset = (tableIdx - 5.5f) * singleTableShortSide; // 居中排列（12张桌子，中心在中间）
                glm::mat4 tableMatrix = glm::mat4(1.0f);
                tableMatrix = glm::translate(tableMatrix, glm::vec3(xOffset, 0.0f, zPos));
                tableMatrix = glm::scale(tableMatrix, glm::vec3(1.2f));
                // 使用PBR材质渲染
                renderModelWithPBRMaterial(pbrShader, libraryTable, oakMat, tableMatrix);
            }
            
            // 在每排桌子的长边均匀放置椅子（长边是X方向，椅子放在X方向的两侧，即左右两侧）
            // 增加椅子数量，并让椅子更靠近桌子
            int chairsPerSide = 8; // 每边8把椅子（增加数量）
            float chairSpacing = totalTableLength / (chairsPerSide + 1); // 椅子间距
            
            // 左侧长边的椅子（面向桌子，X负方向，旋转90度，更靠近桌子）
            for (int chairIdx = 0; chairIdx < chairsPerSide; chairIdx++) {
                float xPos = -totalTableLength / 2.0f + (chairIdx + 1) * chairSpacing; // 沿X轴均匀分布
                float zPos_chair = zPos - totalTableWidth / 2.0f - 0.4f; // 桌子左侧，更靠近（从0.8f改为0.4f）
                
                glm::mat4 stoolMatrix = glm::mat4(1.0f);
                stoolMatrix = glm::translate(stoolMatrix, glm::vec3(xPos, 0.0f, zPos_chair));
                stoolMatrix = glm::rotate(stoolMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 旋转180度，面向桌子
                stoolMatrix = glm::scale(stoolMatrix, glm::vec3(1.0f));
                // 使用PBR材质渲染
                renderModelWithPBRMaterial(pbrShader, stool, leatherMat, stoolMatrix, true);
            }
            
            // 右侧长边的椅子（面向桌子，X正方向，旋转90度，更靠近桌子）
            for (int chairIdx = 0; chairIdx < chairsPerSide; chairIdx++) {
                float xPos = -totalTableLength / 2.0f + (chairIdx + 1) * chairSpacing; // 沿X轴均匀分布
                float zPos_chair = zPos + totalTableWidth / 2.0f + 0.4f; // 桌子右侧，更靠近（从0.8f改为0.4f）
                
                glm::mat4 stoolMatrix = glm::mat4(1.0f);
                stoolMatrix = glm::translate(stoolMatrix, glm::vec3(xPos, 0.0f, zPos_chair));
                stoolMatrix = glm::rotate(stoolMatrix, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 旋转90度（总共0度，即面向桌子）
                stoolMatrix = glm::scale(stoolMatrix, glm::vec3(1.0f));
                // 使用PBR材质渲染
                renderModelWithPBRMaterial(pbrShader, stool, leatherMat, stoolMatrix, true);
            }
        }

        // ========= 渲染天花板 =========
        glm::mat4 ceilingMatrix = glm::mat4(1.0f);
        ceilingMatrix = glm::translate(ceilingMatrix, glm::vec3(0.0f, 6.0f, 0.0f));
        ceilingMatrix = glm::scale(ceilingMatrix, glm::vec3(25.0f, 0.1f, 40.0f));
        renderModelWithPBRMaterial(pbrShader, cube, tileMat, ceilingMatrix, true, false);  // 禁用triplanar

        // ========= 渲染天花板上的灯（2x4排布，共8个灯，每个灯对应一个光源）=========
        // 设置光源数量为8个
        pbrShader.use();
        pbrShader.setInt("lightCount", 8);
        
        lightIndex = 0;
        for (int i = 0; i < lightCountX; i++) {
            for (int j = 0; j < lightCountZ; j++) {
                // 渲染灯模型（使用sphere）
                glm::mat4 lightMatrix = glm::mat4(1.0f);
                lightMatrix = glm::translate(lightMatrix, lightPositions[lightIndex]);
                lightMatrix = glm::scale(lightMatrix, glm::vec3(0.3f, 0.1f, 0.3f));  // 扁平化作为灯
                renderModelWithPBRMaterial(pbrShader, sphere, paintedMetalMat, lightMatrix);
                
                // 在灯的位置添加点光源
                pbrShader.use();
                char lightPosName[32], lightColorName[32], lightIntensityName[32];
                snprintf(lightPosName, sizeof(lightPosName), "lights[%d].position", lightIndex);
                snprintf(lightColorName, sizeof(lightColorName), "lights[%d].color", lightIndex);
                snprintf(lightIntensityName, sizeof(lightIntensityName), "lights[%d].intensity", lightIndex);
                pbrShader.setVec3(lightPosName, lightPositions[lightIndex]);
                pbrShader.setVec3(lightColorName, glm::vec3(1.0f, 0.95f, 0.85f));
                pbrShader.setFloat(lightIntensityName, 200.0f);  // 大幅提高点光源强度（从50.0提高到200.0）
                
                lightIndex++;
            }
        }

        // ========= 渲染饮水机（角落）=========
        glm::mat4 dispenserMatrix = glm::mat4(1.0f);
        dispenserMatrix = glm::translate(dispenserMatrix, glm::vec3(11.0f, 0.0f, 18.0f));  // 调整到角落
        dispenserMatrix = glm::rotate(dispenserMatrix, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        dispenserMatrix = glm::scale(dispenserMatrix, glm::vec3(1.0f));
        // 使用PBR材质渲染
        renderModelWithPBRMaterial(pbrShader, waterDispenser, metalMat, dispenserMatrix);


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
