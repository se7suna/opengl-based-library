#include "ShadowManager.h"
#include <iostream>

ShadowManager::ShadowManager()
    : shadowMapFBO(0), shadowMapTexture(0), shadowMapSize(1024),
      shadowShader(nullptr), shadowBias(0.005f), shadowRange(20.0f),
      currentLightPos(0.0f), currentLightDir(0.0f, -1.0f, 0.0f), isDirectional(true) {
    lightSpaceMatrix = glm::mat4(1.0f);
    lightProjection = glm::mat4(1.0f);
    lightView = glm::mat4(1.0f);
}

ShadowManager::~ShadowManager() {
    Cleanup();
}

void ShadowManager::Initialize(unsigned int size) {
    shadowMapSize = size;
    shadowBias = 0.005f;
    shadowRange = 20.0f;

    // 创建阴影shader
    shadowShader = new Shader("shaders/shadow.vert", "shaders/shadow.frag");

    // 创建阴影贴图FBO
    glGenFramebuffers(1, &shadowMapFBO);

    // 创建阴影贴图纹理（深度贴图）
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 绑定FBO并附加深度纹理
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // 检查FBO完整性
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "阴影贴图FBO创建失败！" << std::endl;
    }

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
    currentLightPos = lightPos;
    currentLightDir = lightDir;
    isDirectional = isDirectionalLight;

    // 计算光源空间矩阵
    CalculateLightSpaceMatrix(lightPos, lightDir, isDirectionalLight);

    // 绑定阴影贴图FBO
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // 使用阴影shader
    shadowShader->use();
    shadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
}

void ShadowManager::EndShadowMapRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    // 重新绑定到FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowManager::UpdateLight(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight) {
    currentLightPos = lightPos;
    currentLightDir = lightDir;
    isDirectional = isDirectionalLight;
    CalculateLightSpaceMatrix(lightPos, lightDir, isDirectionalLight);
}

void ShadowManager::CalculateLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight) {
    if (isDirectionalLight) {
        // 方向光：使用正交投影
        float nearPlane = 0.1f;
        float farPlane = shadowRange;
        float orthoSize = shadowRange * 0.5f;

        lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
        
        // 光源视图矩阵：从光源位置看向场景中心
        glm::vec3 lightTarget = lightPos + lightDir * shadowRange;
        lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
        // 点光源：使用透视投影（未来可扩展为立方体贴图）
        float nearPlane = 0.1f;
        float farPlane = shadowRange;
        float fov = glm::radians(90.0f);

        lightProjection = glm::perspective(fov, 1.0f, nearPlane, farPlane);
        lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    lightSpaceMatrix = lightProjection * lightView;
}

