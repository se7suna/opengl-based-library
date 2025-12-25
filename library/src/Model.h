#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include "Mesh.h"  // 必须包含 Mesh.h（Model 包含 Mesh）

class Model {
public:
    std::vector<Mesh> meshes;

    // 构造函数声明
    Model(const std::string& path);

    // 绘制函数声明
    void Draw(Shader& shader);

private:
    // 声明 loadOBJ 函数（供内部调用）
    void loadOBJ(const std::string& path);
    void calculateNormals(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
};

#endif