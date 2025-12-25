#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <glm/glm.hpp>
#include "Model.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "ProceduralPlant.h"

// 场景类：管理所有场景对象、材质和光照
class Scene {
public:
    Scene();
    ~Scene();

    // 初始化场景（加载模型、材质等）
    void Initialize();

    // 设置光照（在渲染前调用）
    void SetupLighting(Shader& pbrShader);

    // 渲染场景
    void Render(Shader& pbrShader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos);

private:
    // 场景模型
    Model* bookshelf;
    Model* libraryTable;
    Model* stool;
    Model* waterDispenser;
    Model* cube;
    Model* sphere;
    Model* ceilingLamp;

    // PBR 材质
    PBRTextureMaterial oakMat;
    PBRTextureMaterial woodFloorMat;
    PBRTextureMaterial metalMat;
    PBRTextureMaterial paintedMetalMat;
    PBRTextureMaterial leatherMat;
    PBRTextureMaterial tileMat;

    // 程序化生成的盆栽
    std::vector<PottedPlant> plants;

    // 辅助函数：使用PBR材质渲染模型
    void renderModelWithPBRMaterial(Shader& pbrShader, Model& model, PBRTextureMaterial& mat, 
                                     glm::mat4& modelMatrix, bool useGloss = false);

    // 辅助函数：使用PBR材质渲染 Mesh（用于程序化生成物体）
    void renderMeshWithPBRMaterial(Shader& pbrShader, Mesh& mesh, PBRTextureMaterial& mat,
                                    glm::mat4& modelMatrix, bool useGloss = false);
};

#endif

