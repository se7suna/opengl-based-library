#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <glad/glad.h>

// 简单的 PBR 纹理材质结构（基于贴图）
struct PBRTextureMaterial {
    GLuint albedoTex   = 0;
    GLuint normalTex   = 0;
    GLuint metallicTex = 0;
    GLuint roughnessTex= 0;
    GLuint aoTex       = 0;
};

// 从文件加载 2D 纹理
//  - path:   相对于可执行文件的路径
//  - srgb:   是否使用 sRGB 色彩空间（一般只有 Albedo 需要）
// 返回 OpenGL 纹理 ID（失败返回 0）
GLuint LoadTexture2D(const std::string& path, bool srgb);

// 针对 Poliigon_WoodVeneerOak_7760 材质的便捷加载函数
PBRTextureMaterial LoadMaterial_WoodVeneerOak_7760();

#endif


