#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class ShadowManager {
public:
    ShadowManager();
    ~ShadowManager();

    // 初始化阴影贴图
    // shadowMapSize: 阴影贴图分辨率（默认2048x2048）
    void Initialize(unsigned int shadowMapSize = 2048);

    // 清理资源
    void Cleanup();

    // 开始渲染阴影贴图（从光源视角）
    // lightPos: 光源位置
    // lightDir: 光源方向（归一化）
    // isDirectionalLight: 是否为方向光（true）或点光源（false）
    void BeginShadowMapRender(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight = true);

    // 结束阴影贴图渲染
    void EndShadowMapRender();

    // 获取光源空间矩阵（用于在PBR着色器中采样阴影贴图）
    glm::mat4 GetLightSpaceMatrix() const { return lightSpaceMatrix; }

    // 获取阴影贴图纹理ID
    unsigned int GetShadowMapTexture() const { return shadowMapTexture; }

    // 设置阴影贴图大小
    void SetShadowMapSize(unsigned int size);

    // 获取阴影贴图大小
    unsigned int GetShadowMapSize() const { return shadowMapSize; }

    // 设置阴影偏移（避免阴影失真）
    void SetShadowBias(float bias) { shadowBias = bias; }
    float GetShadowBias() const { return shadowBias; }

    // 设置阴影范围（方向光的正交投影范围）
    void SetShadowRange(float range) { shadowRange = range; }
    float GetShadowRange() const { return shadowRange; }

    // 获取阴影着色器（用于渲染阴影贴图）
    Shader* GetShadowShader() const { return shadowShader; }

private:
    // 更新光源空间矩阵
    void CalculateLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight);

    // 阴影贴图FBO和纹理
    unsigned int shadowMapFBO;
    unsigned int shadowMapTexture;
    unsigned int shadowMapSize;

    // 光源空间矩阵
    glm::mat4 lightSpaceMatrix;

    // 阴影着色器
    Shader* shadowShader;

    // 阴影参数
    float shadowBias;
    float shadowRange;
};

#endif // SHADOW_MANAGER_H

