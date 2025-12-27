#include "ShadowManager.h"
#include <glm/gtc/matrix_transform.hpp>

ShadowManager::ShadowManager()
    : shadowMapFBO(0), shadowMapTexture(0), shadowMapSize(2048),
      shadowShader(nullptr), shadowBias(0.005f), shadowRange(20.0f),
      lightSpaceMatrix(1.0f) {
}

ShadowManager::~ShadowManager() {
    Cleanup();
}

void ShadowManager::Initialize(unsigned int size) {
    shadowMapSize = size;
    shadowBias = 0.005f;
    shadowRange = 20.0f;

    // 创建阴影着色器
    shadowShader = new Shader("shaders/shadow.vert", "shaders/shadow.frag");

    // 创建FBO
    glGenFramebuffers(1, &shadowMapFBO);

    // 创建深度纹理
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 绑定FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowManager::Cleanup() {
    if (shadowMapFBO != 0) {
        glDeleteFramebuffers(1, &shadowMapFBO);
        shadowMapFBO = 0;
    }
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
        shadowMapTexture = 0;
    }
    if (shadowShader != nullptr) {
        delete shadowShader;
        shadowShader = nullptr;
    }
}

void ShadowManager::BeginShadowMapRender(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight) {
    // 计算光源空间矩阵
    CalculateLightSpaceMatrix(lightPos, lightDir, isDirectionalLight);

    // 设置视口并绑定FBO
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);  // 使用正面剔除减少阴影失真

    shadowShader->use();
    shadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
}

void ShadowManager::EndShadowMapRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCullFace(GL_BACK);  // 恢复背面剔除
}

void ShadowManager::SetShadowMapSize(unsigned int size) {
    if (size == shadowMapSize) return;

    shadowMapSize = size;

    // 重新创建纹理
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
    }

    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowManager::CalculateLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight) {
    if (isDirectionalLight) {
        // 方向光：使用正交投影
        // 场景范围：15x15单位（x和z方向），高度约5单位
        // 使用足够大的范围覆盖整个场景
        float orthoSize = 8.0f;  // 覆盖±8单位，确保覆盖15x15的场景（从-7.5到+7.5）
        
        // 光源位置和观察目标
        glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // 场景中心
        glm::vec3 lightPos_world = lightTarget - lightDir * shadowRange * 0.5f;
        
        // 计算观察空间中场景的深度范围
        // 在观察空间中，场景中心在深度 shadowRange * 0.5f = 10.0f 处
        // 场景大小：15x15单位，前后各约7.5单位
        // 为了保持深度精度，使用一个紧凑但足够覆盖场景的范围
        float distanceToCenter = shadowRange * 0.5f;  // 约10.0f
        // 使用9单位的半范围，以场景中心为中心（覆盖范围18单位）
        // 场景需要覆盖：10.0 ± 7.5 = 2.5 到 17.5，我们的范围1.0到19.0可以覆盖
        float depthHalfRange = 9.0f;
        float nearPlane = distanceToCenter - depthHalfRange;  // 1.0f
        float farPlane = distanceToCenter + depthHalfRange;   // 19.0f

        // 光源空间视图矩阵
        glm::mat4 lightView = glm::lookAt(lightPos_world, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

        // 正交投影矩阵
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);

        lightSpaceMatrix = lightProjection * lightView;
    } else {
        // 点光源：使用透视投影（如果需要的话）
        float nearPlane = 9.0f;
        float farPlane = shadowRange * 0.5f;

        glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

        lightSpaceMatrix = lightProjection * lightView;
    }
}

