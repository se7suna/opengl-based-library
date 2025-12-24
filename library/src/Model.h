#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "Mesh.h"  // Mesh.hModel Mesh

// MTL材质结构
struct MTLMaterial {
    glm::vec3 Ka;  // 环境光颜色
    glm::vec3 Kd;  // 漫反射颜色
    glm::vec3 Ks;  // 镜面反射颜色
    float Ns;      // 镜面反射指数
    
    MTLMaterial() : Ka(0.2f), Kd(0.8f), Ks(0.1f), Ns(16.0f) {}
};

class Model {
public:
    std::vector<Mesh> meshes;
    std::map<std::string, MTLMaterial> materials;  // 材质映射
    std::string currentMaterial;  // 当前使用的材质名称

    // 构造函数
    Model(const std::string& path);

    // 绘制函数
    void Draw(Shader& shader);
    
    // 获取材质颜色（用于调制PBR材质）
    glm::vec3 getMaterialColor() const;
    
    // 检查是否有有效的MTL材质（不是默认材质）
    bool hasMTLMaterial() const;

private:
    // 内部函数 loadOBJ
    void loadOBJ(const std::string& path);
    void loadMTL(const std::string& path);
    void calculateNormals(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    void calculateTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
};

#endif
