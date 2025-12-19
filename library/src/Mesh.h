#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"  // 必须包含 Shader.h（Draw 函数用 Shader）

// 顶点结构体（全局可见，供 Model 类使用）
struct Vertex {
    glm::vec3 Pos;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

    Vertex() {}
    Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 tex)
        : Pos(pos), Normal(normal), TexCoords(tex) {
    }
};

// Mesh 类声明
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    // 构造函数声明
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

    // 绘制函数声明
    void Draw(Shader& shader);

private:
    void setupMesh();
};

#endif