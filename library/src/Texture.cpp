#include "Texture.h"

#include <iostream>
#include <string>

// 依赖 stb_image 进行图片加载
// 你需要自己把官方的 stb_image.h 放到 external/stb_image.h
// 下载地址： https://github.com/nothings/stb/blob/master/stb_image.h
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

static GLuint CreateGLTexture(unsigned char* data, int width, int height, int nrChannels, bool srgb) {
    if (!data) return 0;

    GLenum internalFormat = GL_RGB;
    GLenum dataFormat = GL_RGB;

    if (nrChannels == 1) {
        internalFormat = dataFormat = GL_RED;
    } else if (nrChannels == 3) {
        internalFormat = srgb ? GL_SRGB : GL_RGB;
        dataFormat = GL_RGB;
    } else if (nrChannels == 4) {
        internalFormat = srgb ? GL_SRGB_ALPHA : GL_RGBA;
        dataFormat = GL_RGBA;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GLuint LoadTexture2D(const std::string& path, bool srgb) {
    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, nrChannels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    GLuint tex = CreateGLTexture(data, width, height, nrChannels, srgb);
    stbi_image_free(data);
    return tex;
}

PBRTextureMaterial LoadMaterial_WoodVeneerOak_7760() {
    PBRTextureMaterial mat{};

    // 注意：这些路径是相对于 exe 的工作目录
    // 运行时需要保证 materials 文件夹与 exe 在同一目录
    mat.albedoTex    = LoadTexture2D("materials/Poliigon_WoodVeneerOak_7760/1K/Poliigon_WoodVeneerOak_7760_BaseColor.jpg", true);
    mat.normalTex    = LoadTexture2D("materials/Poliigon_WoodVeneerOak_7760/1K/Poliigon_WoodVeneerOak_7760_Normal.png", false);
    mat.metallicTex  = LoadTexture2D("materials/Poliigon_WoodVeneerOak_7760/1K/Poliigon_WoodVeneerOak_7760_Metallic.jpg", false);
    mat.roughnessTex = LoadTexture2D("materials/Poliigon_WoodVeneerOak_7760/1K/Poliigon_WoodVeneerOak_7760_Roughness.jpg", false);
    mat.aoTex        = LoadTexture2D("materials/Poliigon_WoodVeneerOak_7760/1K/Poliigon_WoodVeneerOak_7760_AmbientOcclusion.jpg", false);

    return mat;
}

PBRTextureMaterial LoadMaterial_WoodFloorAsh_4186() {
    PBRTextureMaterial mat{};
    mat.albedoTex    = LoadTexture2D("materials/Poliigon_WoodFloorAsh_4186_Preview1/1K/Poliigon_WoodFloorAsh_4186_BaseColor.jpg", true);
    mat.normalTex    = LoadTexture2D("materials/Poliigon_WoodFloorAsh_4186_Preview1/1K/Poliigon_WoodFloorAsh_4186_Normal.png", false);
    mat.metallicTex  = LoadTexture2D("materials/Poliigon_WoodFloorAsh_4186_Preview1/1K/Poliigon_WoodFloorAsh_4186_Metallic.jpg", false);
    mat.roughnessTex = LoadTexture2D("materials/Poliigon_WoodFloorAsh_4186_Preview1/1K/Poliigon_WoodFloorAsh_4186_Roughness.jpg", false);
    mat.aoTex        = LoadTexture2D("materials/Poliigon_WoodFloorAsh_4186_Preview1/1K/Poliigon_WoodFloorAsh_4186_AmbientOcclusion.jpg", false);
    return mat;
}

PBRTextureMaterial LoadMaterial_MetalGalvanizedZinc_7184() {
    PBRTextureMaterial mat{};
    mat.albedoTex    = LoadTexture2D("materials/Poliigon_MetalGalvanizedZinc_7184/512/Poliigon_MetalGalvanizedZinc_7184_BaseColor.jpg", true);
    mat.normalTex    = LoadTexture2D("materials/Poliigon_MetalGalvanizedZinc_7184/512/Poliigon_MetalGalvanizedZinc_7184_Normal.png", false);
    mat.metallicTex  = LoadTexture2D("materials/Poliigon_MetalGalvanizedZinc_7184/512/Poliigon_MetalGalvanizedZinc_7184_Metallic.jpg", false);
    mat.roughnessTex = LoadTexture2D("materials/Poliigon_MetalGalvanizedZinc_7184/512/Poliigon_MetalGalvanizedZinc_7184_Roughness.jpg", false);
    mat.aoTex        = LoadTexture2D("materials/Poliigon_MetalGalvanizedZinc_7184/512/Poliigon_MetalGalvanizedZinc_7184_AmbientOcclusion.jpg", false);
    return mat;
}

PBRTextureMaterial LoadMaterial_MetalPaintedMatte_7037() {
    PBRTextureMaterial mat{};
    mat.albedoTex    = LoadTexture2D("materials/Poliigon_MetalPaintedMatte_7037_Preview1/1K/Poliigon_MetalPaintedMatte_7037_BaseColor.jpg", true);
    mat.normalTex    = LoadTexture2D("materials/Poliigon_MetalPaintedMatte_7037_Preview1/1K/Poliigon_MetalPaintedMatte_7037_Normal.png", false);
    mat.metallicTex  = LoadTexture2D("materials/Poliigon_MetalPaintedMatte_7037_Preview1/1K/Poliigon_MetalPaintedMatte_7037_Metallic.jpg", false);
    mat.roughnessTex = LoadTexture2D("materials/Poliigon_MetalPaintedMatte_7037_Preview1/1K/Poliigon_MetalPaintedMatte_7037_Roughness.jpg", false);
    mat.aoTex        = LoadTexture2D("materials/Poliigon_MetalPaintedMatte_7037_Preview1/1K/Poliigon_MetalPaintedMatte_7037_AmbientOcclusion.jpg", false);
    return mat;
}

PBRTextureMaterial LoadMaterial_FabricLeatherCowhide_001() {
    PBRTextureMaterial mat{};
    // FabricLeatherCowhide 使用 COL 作为 BaseColor，REFL 作为 Metallic，GLOSS 作为 Roughness（需要反转，但这里先直接用，shader中处理）
    mat.albedoTex    = LoadTexture2D("materials/FabricLeatherCowhide001/FabricLeatherCowhide001_COL_VAR1_1K.jpg", true);
    mat.normalTex    = LoadTexture2D("materials/FabricLeatherCowhide001/FabricLeatherCowhide001_NRM_1K.jpg", false);
    mat.metallicTex  = LoadTexture2D("materials/FabricLeatherCowhide001/FabricLeatherCowhide001_REFL_1K.jpg", false);
    // GLOSS 是光滑度，Roughness 是粗糙度，所以 GLOSS = 1 - Roughness，但这里先直接使用
    // 注意：shader中可能需要反转，或者这里创建一个反转的纹理，暂时直接使用
    mat.roughnessTex = LoadTexture2D("materials/FabricLeatherCowhide001/FabricLeatherCowhide001_GLOSS_1K.jpg", false);
    mat.aoTex        = LoadTexture2D("materials/FabricLeatherCowhide001/FabricLeatherCowhide001_AO_1K.jpg", false);
    return mat;
}

PBRTextureMaterial LoadMaterial_TilesTravertine_001() {
    PBRTextureMaterial mat{};
    // TilesTravertine 使用 COL 作为 BaseColor，REFL 作为 Metallic，GLOSS 作为 Roughness
    mat.albedoTex    = LoadTexture2D("materials/TilesTravertine001/TilesTravertine001_COL_1K.jpg", true);
    mat.normalTex    = LoadTexture2D("materials/TilesTravertine001/TilesTravertine001_NRM_1K.jpg", false);
    mat.metallicTex  = LoadTexture2D("materials/TilesTravertine001/TilesTravertine001_REFL_1K.jpg", false);
    // GLOSS 是光滑度，Roughness 是粗糙度，暂时直接使用
    mat.roughnessTex = LoadTexture2D("materials/TilesTravertine001/TilesTravertine001_GLOSS_1K.jpg", false);
    mat.aoTex        = LoadTexture2D("materials/TilesTravertine001/TilesTravertine001_AO_1K.jpg", false);
    return mat;
}


