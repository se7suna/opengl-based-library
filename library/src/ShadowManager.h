#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

// 阴影管理器类：支持动态光照和阴影映射
class ShadowManager {
public:
    ShadowManager();
    ~ShadowManager();

    // 初始化阴影贴图（在场景初始化时调用）
    // shadowMapSize: 阴影贴图分辨率（默认1024x1024）
    void Initialize(unsigned int shadowMapSize = 1024);

    // 清理资源
    void Cleanup();

    // 开始渲染阴影贴图（在渲染场景前调用）
    // lightPos: 光源位置（世界空间）
    // lightDir: 光源方向（归一化向量，用于方向光）
    // isDirectionalLight: true为方向光，false为点光源
    void BeginShadowMapRender(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight = true);

    // 结束阴影贴图渲染
    void EndShadowMapRender();

    // 获取光源空间变换矩阵（用于主渲染pass）
    glm::mat4 GetLightSpaceMatrix() const { return lightSpaceMatrix; }

    // 获取阴影贴图纹理ID（用于绑定到shader）
    unsigned int GetShadowMapTexture() const { return shadowMapTexture; }

    // 设置阴影贴图分辨率
    void SetShadowMapSize(unsigned int size);

    // 获取阴影贴图分辨率
    unsigned int GetShadowMapSize() const { return shadowMapSize; }

    // 设置阴影参数
    void SetShadowBias(float bias) { shadowBias = bias; }
    float GetShadowBias() const { return shadowBias; }

    // 设置阴影范围（方向光使用）
    void SetShadowRange(float range) { shadowRange = range; }
    float GetShadowRange() const { return shadowRange; }

    // 更新光源位置和方向（用于动态光源）
    void UpdateLight(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight = true);

private:
    // 阴影贴图FBO
    unsigned int shadowMapFBO;
    unsigned int shadowMapTexture;
    unsigned int shadowMapSize;

    // 阴影shader
    Shader* shadowShader;

    // 光源空间变换矩阵
    glm::mat4 lightSpaceMatrix;
    glm::mat4 lightProjection;
    glm::mat4 lightView;

    // 阴影参数
    float shadowBias;
    float shadowRange;

    // 当前光源信息
    glm::vec3 currentLightPos;
    glm::vec3 currentLightDir;
    bool isDirectional;

    // 计算光源空间矩阵
    void CalculateLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& lightDir, bool isDirectionalLight);
};

#endif // SHADOW_MANAGER_H

