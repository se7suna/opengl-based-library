#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"  // ������� Shader.h��Draw ������ Shader��

// ����ṹ�壨ȫ�ֿɼ����� Model ��ʹ�ã�
struct Vertex {
    glm::vec3 Pos;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;  // 切线（用于法线贴图）

    Vertex() {}
    Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 tex)
        : Pos(pos), Normal(normal), TexCoords(tex), Tangent(0.0f) {
    }
    Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 tex, glm::vec3 tangent)
        : Pos(pos), Normal(normal), TexCoords(tex), Tangent(tangent) {
    }
};

// Mesh ������
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    // ���캯������
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

    // ���ƺ�������
    void Draw(Shader& shader);

private:
    void setupMesh();
};

#endif