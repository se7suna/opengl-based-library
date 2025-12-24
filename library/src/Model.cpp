#include "Model.h"
#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <glm/glm.hpp>

Model::Model(const std::string& path) {
    currentMaterial = "default";
    materials["default"] = MTLMaterial();  // 默认材质
    loadOBJ(path);
}

glm::vec3 Model::getMaterialColor() const {
    auto it = materials.find(currentMaterial);
    if (it != materials.end()) {
        return it->second.Kd;  // 返回漫反射颜色
    }
    return glm::vec3(0.8f);  // 默认颜色
}

bool Model::hasMTLMaterial() const {
    // 检查是否有除了"default"之外的材质，或者default材质是否被修改过
    if (materials.size() > 1) {
        return true;  // 有多个材质，说明加载了mtl文件
    }
    // 检查default材质是否与默认值不同（说明可能从mtl文件加载了值）
    auto it = materials.find("default");
    if (it != materials.end()) {
        // 如果Kd不是默认的0.8，说明可能从mtl加载了
        return it->second.Kd != glm::vec3(0.8f);
    }
    return false;
}

void Model::Draw(Shader& shader) {
    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].Draw(shader);
    }
}

// 加载MTL材质文件
void Model::loadMTL(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "WARNING::MODEL::MTL_FILE_OPEN_FAILED: " << path << std::endl;
        return;
    }
    
    std::string currentMtlName = "default";
    std::string line;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "newmtl") {
            iss >> currentMtlName;
            materials[currentMtlName] = MTLMaterial();  // 创建新材质
        }
        else if (type == "Ka") {
            glm::vec3 ka;
            iss >> ka.r >> ka.g >> ka.b;
            materials[currentMtlName].Ka = ka;
        }
        else if (type == "Kd") {
            glm::vec3 kd;
            iss >> kd.r >> kd.g >> kd.b;
            materials[currentMtlName].Kd = kd;
        }
        else if (type == "Ks") {
            glm::vec3 ks;
            iss >> ks.r >> ks.g >> ks.b;
            materials[currentMtlName].Ks = ks;
        }
        else if (type == "Ns") {
            float ns;
            iss >> ns;
            materials[currentMtlName].Ns = ns;
        }
    }
    
    file.close();
}

// 彻底修复的OBJ解析逻辑（重点：法线索引提取 + 纹理坐标解析）
void Model::loadOBJ(const std::string& path) {
    std::vector<glm::vec3> temp_vertices;  // 临时顶点位置
    std::vector<glm::vec2> temp_texCoords; // 临时纹理坐标
    std::vector<glm::vec3> temp_normals;   // 临时法线
    std::vector<unsigned int> vIndices, tIndices, nIndices;  // 顶点/纹理/法线索引

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR::MODEL::FILE_OPEN_FAILED: " << path << std::endl;
        return;
    }
    
    // 获取OBJ文件所在目录
    std::string objDir = ".";
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        objDir = path.substr(0, lastSlash);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        // 处理mtllib指令
        if (type == "mtllib") {
            std::string mtlFileName;
            iss >> mtlFileName;
            std::string mtlPath = objDir + "/" + mtlFileName;
            loadMTL(mtlPath);
        }
        // 处理usemtl指令
        else if (type == "usemtl") {
            std::string mtlName;
            iss >> mtlName;
            currentMaterial = mtlName;
        }

        // 1. 解析顶点位置（v x y z）
        if (type == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        // 2. 解析纹理坐标（vt u v）
        else if (type == "vt") {
            glm::vec2 vt;
            iss >> vt.x >> vt.y;
            temp_texCoords.push_back(vt);
        }
        // 3. 解析法线（vn x y z）
        else if (type == "vn") {
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        // 4. 解析面（支持多种格式：f v1 v2 v3 或 f v1/vt1 v2/vt2 v3/vt3 或 f v1//vn1 或 f v1/vt1/vn1）
        else if (type == "f") {
            std::string vertexStr;
            std::vector<std::string> faceVertices;
            
            // 读取面的所有顶点（不假设只有3个，但只处理前3个）
            while (iss >> vertexStr) {
                faceVertices.push_back(vertexStr);
            }
            
            // 只处理三角形面（至少3个顶点）
            if (faceVertices.size() < 3) {
                std::cerr << "WARNING::MODEL::INVALID_FACE: face has less than 3 vertices in " << path << std::endl;
                continue;
            }
            
            // 处理前3个顶点（如果是多边形，只取前3个构成三角形）
            for (int i = 0; i < 3; i++) {
                std::istringstream vss(faceVertices[i]);
                std::string part;
                unsigned int vIdx = 0;
                bool hasTexIndex = false;
                unsigned int tIdx = 0;
                bool hasNormalIndex = false;
                unsigned int nIdx = 0;

                // 分割逻辑：处理多种格式
                // "v" 或 "v/vt" 或 "v//vn" 或 "v/vt/vn"
                int partCount = 0;
                while (std::getline(vss, part, '/')) {
                    partCount++;
                    if (partCount == 1 && !part.empty()) {  // 第一个部分：顶点索引
                        try {
                            vIdx = std::stoi(part) - 1;  // OBJ索引1→0
                        } catch (const std::exception& e) {
                            std::cerr << "WARNING::MODEL::INVALID_VERTEX_FORMAT: " << part << " in " << path << std::endl;
                            vIdx = 0;
                        }
                    }
                    else if (partCount == 2 && !part.empty()) {  // 第二个部分：纹理坐标索引
                        try {
                            tIdx = std::stoi(part) - 1;
                            hasTexIndex = true;
                        } catch (const std::exception& e) {
                            // 无法解析纹理坐标索引，忽略
                        }
                    }
                    else if (partCount == 3 && !part.empty()) {  // 第三个部分：法线索引
                        try {
                            nIdx = std::stoi(part) - 1;
                            hasNormalIndex = true;
                        } catch (const std::exception& e) {
                            // 无法解析法线索引，忽略
                        }
                    }
                }

                // 验证顶点索引是否有效
                if (vIdx >= temp_vertices.size()) {
                    std::cerr << "WARNING::MODEL::INVALID_VERTEX_INDEX: " << vIdx << " (max: " << temp_vertices.size() - 1 << ") in " << path << std::endl;
                    continue;
                }
                
                vIndices.push_back(vIdx);
                
                // 如果有纹理坐标索引，验证并保存
                if (hasTexIndex) {
                    if (tIdx >= temp_texCoords.size()) {
                        std::cerr << "WARNING::MODEL::INVALID_TEXCOORD_INDEX: " << tIdx << " (max: " << temp_texCoords.size() - 1 << ") in " << path << std::endl;
                        tIndices.push_back(0);  // 占位符
                    } else {
                        tIndices.push_back(tIdx);
                    }
                } else {
                    // 没有纹理坐标索引，添加占位符
                    tIndices.push_back(0);  // 占位符，后续会用默认值或生成
                }
                
                // 如果有法线索引，验证并保存
                if (hasNormalIndex) {
                    if (nIdx >= temp_normals.size()) {
                        std::cerr << "WARNING::MODEL::INVALID_NORMAL_INDEX: " << nIdx << " (max: " << temp_normals.size() - 1 << ") in " << path << std::endl;
                        nIndices.push_back(0);  // 占位符
                    } else {
                        nIndices.push_back(nIdx);
                    }
                } else {
                    // 没有法线索引，添加占位符
                    nIndices.push_back(0);  // 占位符
                }
            }
        }
    }
    file.close();

    // 检查是否有法线数据
    bool hasNormals = !temp_normals.empty() && !nIndices.empty();
    if (hasNormals) {
        bool hasValidNormalIndices = false;
        for (unsigned int i = 0; i < nIndices.size(); i++) {
            if (nIndices[i] < temp_normals.size()) {
                hasValidNormalIndices = true;
                break;
            }
        }
        hasNormals = hasValidNormalIndices;
    }

    // 检查是否有纹理坐标数据
    bool hasTexCoords = !temp_texCoords.empty() && !tIndices.empty();
    if (hasTexCoords) {
        bool hasValidTexIndices = false;
        for (unsigned int i = 0; i < tIndices.size(); i++) {
            if (tIndices[i] < temp_texCoords.size()) {
                hasValidTexIndices = true;
                break;
            }
        }
        hasTexCoords = hasValidTexIndices;
    }

    // 构建顶点数据
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    // 确保索引数组大小一致
    if (vIndices.size() != tIndices.size() || vIndices.size() != nIndices.size()) {
        std::cerr << "ERROR::MODEL::INDEX_MISMATCH: vIndices(" << vIndices.size() 
                  << "), tIndices(" << tIndices.size() 
                  << "), nIndices(" << nIndices.size() << ") have different sizes in " << path << std::endl;
        return;
    }
    
    for (unsigned int i = 0; i < vIndices.size(); i++) {
        Vertex vert;
        vert.Pos = temp_vertices[vIndices[i]];
        
        // 设置法线
        if (hasNormals && nIndices[i] < temp_normals.size()) {
            vert.Normal = temp_normals[nIndices[i]];
        } else {
            vert.Normal = glm::vec3(0.0f);  // 临时值，稍后计算
        }
        
        // 设置纹理坐标
        if (hasTexCoords && tIndices[i] < temp_texCoords.size()) {
            vert.TexCoords = temp_texCoords[tIndices[i]];
        } else {
            // 如果没有纹理坐标，使用顶点位置的 XY 作为简单的 UV 映射（适用于简单几何体）
            vert.TexCoords = glm::vec2(vert.Pos.x, vert.Pos.y);
        }
        
        // 初始化切线为零向量（稍后计算）
        vert.Tangent = glm::vec3(0.0f);
        
        vertices.push_back(vert);
        indices.push_back(i);
    }
    
    // 如果无法线数据，自动计算
    if (!hasNormals) {
        if (!vertices.empty() && !indices.empty()) {
            calculateNormals(vertices, indices);
            std::cout << "MODEL::LOADED: " << path << " | Normals calculated automatically" << std::endl;
        }
    } else {
        std::cout << "MODEL::LOADED: " << path << " | Using normals from OBJ file" << std::endl;
    }
    
    // 计算切线（用于法线贴图）
    if (!vertices.empty() && !indices.empty() && hasTexCoords) {
        calculateTangents(vertices, indices);
        std::cout << "MODEL::LOADED: " << path << " | Tangents calculated for normal mapping" << std::endl;
    }
    
    if (hasTexCoords) {
        std::cout << "MODEL::LOADED: " << path << " | Using texture coordinates from OBJ file" << std::endl;
    } else {
        std::cout << "MODEL::LOADED: " << path << " | Generated texture coordinates from vertex positions" << std::endl;
    }

    std::cout << "MODEL::LOADED: " << path << " | Vertices: " << vertices.size() << ", Indices: " << indices.size() << std::endl;
    meshes.push_back(Mesh(vertices, indices));
}

// 自动计算法线：当 OBJ 文件没有法线数据时使用
void Model::calculateNormals(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    // 初始化所有法线为零向量
    for (auto& v : vertices) {
        v.Normal = glm::vec3(0.0f);
    }
    
    // 检查索引数量是否为3的倍数（三角形）
    if (indices.size() % 3 != 0) {
        std::cerr << "WARNING::MODEL::calculateNormals: indices size is not multiple of 3" << std::endl;
    }
    
    // 遍历每个三角形，计算面法线并累加到顶点法线
    for (unsigned int i = 0; i < indices.size(); i += 3) {
        // 检查是否有足够的索引
        if (i + 2 >= indices.size()) {
            std::cerr << "WARNING::MODEL::calculateNormals: incomplete triangle at index " << i << std::endl;
            break;
        }
        
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        // 检查索引是否有效
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            std::cerr << "WARNING::MODEL::calculateNormals: invalid vertex index in triangle at " << i << std::endl;
            continue;
        }
        
        // 计算三角形的两条边
        glm::vec3 v1 = vertices[i1].Pos - vertices[i0].Pos;
        glm::vec3 v2 = vertices[i2].Pos - vertices[i0].Pos;
        
        // 计算面法线（叉积）
        glm::vec3 normal = glm::cross(v1, v2);
        
        // 避免零向量（退化三角形）
        float length = glm::length(normal);
        if (length > 0.0001f) {
            normal = glm::normalize(normal);
            // 将面法线累加到三个顶点的法线
            vertices[i0].Normal += normal;
            vertices[i1].Normal += normal;
            vertices[i2].Normal += normal;
        }
    }
    
    // 归一化所有顶点法线
    for (auto& v : vertices) {
        float length = glm::length(v.Normal);
        if (length > 0.0001f) {
            v.Normal = glm::normalize(v.Normal);
        } else {
            // 如果法线仍为零向量（可能所有三角形都退化），设置为默认向上
            v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

// 计算切线：用于法线贴图的TBN矩阵
void Model::calculateTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    // 初始化所有切线为零向量
    for (auto& v : vertices) {
        v.Tangent = glm::vec3(0.0f);
    }
    
    // 检查索引数量是否为3的倍数（三角形）
    if (indices.size() % 3 != 0) {
        std::cerr << "WARNING::MODEL::calculateTangents: indices size is not multiple of 3" << std::endl;
    }
    
    // 遍历每个三角形，计算切线
    for (unsigned int i = 0; i < indices.size(); i += 3) {
        // 检查是否有足够的索引
        if (i + 2 >= indices.size()) {
            break;
        }
        
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        // 检查索引是否有效
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            continue;
        }
        
        Vertex& v0 = vertices[i0];
        Vertex& v1 = vertices[i1];
        Vertex& v2 = vertices[i2];
        
        // 计算三角形的边向量
        glm::vec3 edge1 = v1.Pos - v0.Pos;
        glm::vec3 edge2 = v2.Pos - v0.Pos;
        
        // 计算UV差值
        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;
        
        // 计算切线和副切线（使用Mikkelsen的方法）
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        
        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        
        // 避免零向量
        float length = glm::length(tangent);
        if (length > 0.0001f) {
            tangent = glm::normalize(tangent);
            // 将切线累加到三个顶点
            v0.Tangent += tangent;
            v1.Tangent += tangent;
            v2.Tangent += tangent;
        }
    }
    
    // 归一化所有顶点切线，并进行Gram-Schmidt正交化
    for (auto& v : vertices) {
        float length = glm::length(v.Tangent);
        if (length > 0.0001f) {
            v.Tangent = glm::normalize(v.Tangent);
            
            // Gram-Schmidt正交化：确保切线与法线垂直
            v.Tangent = glm::normalize(v.Tangent - glm::dot(v.Tangent, v.Normal) * v.Normal);
        } else {
            // 如果切线仍为零向量，生成一个默认切线
            // 使用法线计算一个垂直的切线
            if (abs(v.Normal.x) < 0.9f) {
                v.Tangent = glm::normalize(glm::cross(v.Normal, glm::vec3(1.0f, 0.0f, 0.0f)));
            } else {
                v.Tangent = glm::normalize(glm::cross(v.Normal, glm::vec3(0.0f, 0.0f, 1.0f)));
            }
        }
    }
}