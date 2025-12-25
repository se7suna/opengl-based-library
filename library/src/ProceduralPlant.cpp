#include "ProceduralPlant.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

constexpr float kPi = 3.14159265359f;

GLuint CreateSolidColorTexture2D(unsigned char r, unsigned char g, unsigned char b, unsigned char a, bool srgb) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    const unsigned char pixel[4] = {r, g, b, a};
    const GLenum internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GLuint CreateSolidGrayTexture2D(unsigned char v, bool srgb) {
    return CreateSolidColorTexture2D(v, v, v, 255, srgb);
}

PBRTextureMaterial CreateSolidPBRMaterial(
    const glm::vec3& albedoSrgb01,
    float metallic01,
    float roughness01,
    float ao01) {

    auto toU8 = [](float x) -> unsigned char {
        x = std::clamp(x, 0.0f, 1.0f);
        return static_cast<unsigned char>(std::round(x * 255.0f));
    };

    PBRTextureMaterial mat{};
    mat.albedoTex = CreateSolidColorTexture2D(
        toU8(albedoSrgb01.r),
        toU8(albedoSrgb01.g),
        toU8(albedoSrgb01.b),
        255,
        true);

    // Flat normal map (0.5, 0.5, 1.0)
    mat.normalTex = CreateSolidColorTexture2D(128, 128, 255, 255, false);

    mat.metallicTex = CreateSolidGrayTexture2D(toU8(metallic01), false);
    mat.roughnessTex = CreateSolidGrayTexture2D(toU8(roughness01), false);
    mat.aoTex = CreateSolidGrayTexture2D(toU8(ao01), false);
    return mat;
}

void AppendTriangle(std::vector<unsigned int>& indices, unsigned int a, unsigned int b, unsigned int c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
}

void AppendQuad(std::vector<unsigned int>& indices, unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
    // a-b-c and a-c-d
    AppendTriangle(indices, a, b, c);
    AppendTriangle(indices, a, c, d);
}

void BuildFrustumWall(
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    float y0,
    float y1,
    float r0,
    float r1,
    int segments,
    bool inwardNormals,
    float uScale,
    float vScale,
    float uOffset,
    float vOffset) {

    const unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

    const float slope = (r0 - r1) / std::max(0.0001f, (y1 - y0));

    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float ang = t * 2.0f * kPi;
        const float cs = std::cos(ang);
        const float sn = std::sin(ang);

        const glm::vec3 n = glm::normalize(glm::vec3(cs, slope, sn)) * (inwardNormals ? -1.0f : 1.0f);

        Vertex v0;
        v0.Pos = glm::vec3(cs * r0, y0, sn * r0);
        v0.Normal = n;
        v0.TexCoords = glm::vec2(uOffset + t * uScale, vOffset + 0.0f * vScale);

        Vertex v1;
        v1.Pos = glm::vec3(cs * r1, y1, sn * r1);
        v1.Normal = n;
        v1.TexCoords = glm::vec2(uOffset + t * uScale, vOffset + 1.0f * vScale);

        vertices.push_back(v0);
        vertices.push_back(v1);
    }

    for (int i = 0; i < segments; ++i) {
        const unsigned int i0 = baseIndex + static_cast<unsigned int>(i * 2);
        const unsigned int i1 = i0 + 1;
        const unsigned int i2 = i0 + 3;
        const unsigned int i3 = i0 + 2;

        if (!inwardNormals) {
            AppendQuad(indices, i0, i1, i2, i3);
        } else {
            // flip winding
            AppendQuad(indices, i0, i3, i2, i1);
        }
    }
}

void BuildDisk(
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    float y,
    float r,
    int segments,
    bool flipNormal,
    float uvScale) {

    const unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

    Vertex center;
    center.Pos = glm::vec3(0.0f, y, 0.0f);
    center.Normal = flipNormal ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    center.TexCoords = glm::vec2(0.5f, 0.5f);
    vertices.push_back(center);

    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float ang = t * 2.0f * kPi;
        const float cs = std::cos(ang);
        const float sn = std::sin(ang);

        Vertex v;
        v.Pos = glm::vec3(cs * r, y, sn * r);
        v.Normal = center.Normal;
        v.TexCoords = glm::vec2(0.5f + cs * 0.5f * uvScale, 0.5f + sn * 0.5f * uvScale);
        vertices.push_back(v);
    }

    for (int i = 0; i < segments; ++i) {
        const unsigned int c = baseIndex;
        const unsigned int a = baseIndex + 1 + static_cast<unsigned int>(i);
        const unsigned int b = a + 1;

        if (!flipNormal) {
            AppendTriangle(indices, c, a, b);
        } else {
            AppendTriangle(indices, c, b, a);
        }
    }
}

void BuildLeafRibbon(
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    const glm::vec3& basePos,
    const glm::vec3& dirXZ,
    float length,
    float width,
    float height,
    float curl,
    int segments,
    float twistRadians) {

    const unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const glm::vec3 dir = glm::normalize(glm::vec3(dirXZ.x, 0.0f, dirXZ.z));

    std::vector<glm::vec3> centers;
    centers.reserve(static_cast<size_t>(segments) + 1);

    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float arch = std::sin(t * kPi);

        // center curve in world space
        glm::vec3 p = basePos;
        p += dir * (t * length);
        p.y += t * height + arch * (0.25f * height);

        // sideways curl
        const glm::vec3 side0 = glm::normalize(glm::cross(up, dir));
        p += side0 * (arch * curl);

        centers.push_back(p);
    }

    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);

        const glm::vec3 prev = (i == 0) ? centers[i] : centers[i - 1];
        const glm::vec3 next = (i == segments) ? centers[i] : centers[i + 1];
        glm::vec3 tangent = next - prev;
        if (glm::length(tangent) < 0.0001f) tangent = dir;
        tangent = glm::normalize(tangent);

        glm::vec3 side = glm::cross(up, tangent);
        if (glm::length(side) < 0.0001f) side = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), tangent);
        side = glm::normalize(side);

        // twist around tangent
        const float ang = twistRadians * t;
        const float cs = std::cos(ang);
        const float sn = std::sin(ang);
        glm::vec3 sideTwisted = side * cs + glm::cross(tangent, side) * sn + tangent * glm::dot(tangent, side) * (1.0f - cs);

        const glm::vec3 normal = glm::normalize(glm::cross(tangent, sideTwisted));

        const glm::vec3 left = centers[i] - sideTwisted * (0.5f * width);
        const glm::vec3 right = centers[i] + sideTwisted * (0.5f * width);

        Vertex vl;
        vl.Pos = left;
        vl.Normal = normal;
        vl.TexCoords = glm::vec2(0.0f, t);

        Vertex vr;
        vr.Pos = right;
        vr.Normal = normal;
        vr.TexCoords = glm::vec2(1.0f, t);

        vertices.push_back(vl);
        vertices.push_back(vr);
    }

    for (int i = 0; i < segments; ++i) {
        const unsigned int i0 = baseIndex + static_cast<unsigned int>(i * 2);
        const unsigned int i1 = i0 + 1;
        const unsigned int i2 = i0 + 3;
        const unsigned int i3 = i0 + 2;
        AppendQuad(indices, i0, i1, i2, i3);

        // back face (simple two-sided so leaves look better)
        AppendQuad(indices, i0, i3, i2, i1);
    }
}

} // namespace

PottedPlant CreatePottedPlant(unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> uf01(0.0f, 1.0f);

    // ---- Pot parameters (in meters-ish units, consistent with your scene) ----
    const float potHeight = 0.35f + uf01(rng) * 0.08f;
    const float potTopR = 0.20f + uf01(rng) * 0.04f;
    const float potBottomR = 0.14f + uf01(rng) * 0.03f;
    const float potThickness = 0.025f;
    const float potInnerTopR = std::max(0.02f, potTopR - potThickness);
    const float potInnerBottomR = std::max(0.02f, potBottomR - potThickness);
    const int seg = 48;

    // ---- Build pot mesh ----
    std::vector<Vertex> potVerts;
    std::vector<unsigned int> potIdx;

    // outer wall
    BuildFrustumWall(potVerts, potIdx, 0.0f, potHeight, potBottomR, potTopR, seg, false, 1.0f, 1.0f, 0.0f, 0.0f);

    // inner wall (inward normals)
    BuildFrustumWall(potVerts, potIdx, 0.0f, potHeight * 0.96f, potInnerBottomR, potInnerTopR, seg, true, 1.0f, 1.0f, 0.0f, 0.0f);

    // bottom cap (outer)
    BuildDisk(potVerts, potIdx, 0.0f, potBottomR, seg, true, 1.0f);

    // rim ring (connect outer top to inner top)
    {
        const unsigned int baseIndex = static_cast<unsigned int>(potVerts.size());
        for (int i = 0; i <= seg; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(seg);
            const float ang = t * 2.0f * kPi;
            const float cs = std::cos(ang);
            const float sn = std::sin(ang);

            Vertex vo;
            vo.Pos = glm::vec3(cs * potTopR, potHeight, sn * potTopR);
            vo.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vo.TexCoords = glm::vec2(t, 0.0f);

            Vertex vi;
            vi.Pos = glm::vec3(cs * potInnerTopR, potHeight * 0.96f, sn * potInnerTopR);
            vi.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vi.TexCoords = glm::vec2(t, 1.0f);

            potVerts.push_back(vo);
            potVerts.push_back(vi);
        }

        for (int i = 0; i < seg; ++i) {
            const unsigned int i0 = baseIndex + static_cast<unsigned int>(i * 2);
            const unsigned int i1 = i0 + 1;
            const unsigned int i2 = i0 + 3;
            const unsigned int i3 = i0 + 2;
            AppendQuad(potIdx, i0, i1, i2, i3);
        }
    }

    // ---- Build soil mesh ----
    std::vector<Vertex> soilVerts;
    std::vector<unsigned int> soilIdx;
    const float soilY = potHeight * 0.90f;
    const float soilR = potInnerTopR * 0.92f;
    BuildDisk(soilVerts, soilIdx, soilY, soilR, seg, false, 1.0f);

    // ---- Build leaves mesh ----
    std::vector<Vertex> leafVerts;
    std::vector<unsigned int> leafIdx;

    const int leafCount = 18 + static_cast<int>(uf01(rng) * 12.0f);
    for (int i = 0; i < leafCount; ++i) {
        const float ang = (static_cast<float>(i) / static_cast<float>(leafCount)) * 2.0f * kPi + uf01(rng) * 0.25f;
        glm::vec3 dir(std::cos(ang), 0.0f, std::sin(ang));

        const float len = 0.35f + uf01(rng) * 0.25f;
        const float w = 0.03f + uf01(rng) * 0.02f;
        const float h = 0.25f + uf01(rng) * 0.30f;
        const float curl = 0.04f + uf01(rng) * 0.05f;
        const float twist = (-0.4f + uf01(rng) * 0.8f);
        const int ribbonSeg = 10;

        glm::vec3 basePos(0.0f, soilY + 0.01f, 0.0f);
        basePos += dir * (uf01(rng) * soilR * 0.35f);

        BuildLeafRibbon(leafVerts, leafIdx, basePos, dir, len, w, h, curl, ribbonSeg, twist);
    }

    // ---- Materials ----
    // Pot: warm terracotta; Soil: dark; Leaves: saturated green
    const glm::vec3 potColor = glm::vec3(0.75f, 0.42f, 0.28f) * (0.90f + uf01(rng) * 0.15f);
    const glm::vec3 soilColor = glm::vec3(0.12f, 0.08f, 0.05f) * (0.85f + uf01(rng) * 0.20f);
    const glm::vec3 leafColor = glm::vec3(0.10f, 0.55f, 0.20f) * (0.85f + uf01(rng) * 0.25f);

    PottedPlant plant;
    plant.pot = std::make_shared<Mesh>(potVerts, potIdx);
    plant.soil = std::make_shared<Mesh>(soilVerts, soilIdx);
    plant.leaves = std::make_shared<Mesh>(leafVerts, leafIdx);

    plant.potMat = CreateSolidPBRMaterial(potColor, 0.0f, 0.78f, 1.0f);
    plant.soilMat = CreateSolidPBRMaterial(soilColor, 0.0f, 1.0f, 1.0f);
    plant.leavesMat = CreateSolidPBRMaterial(leafColor, 0.0f, 0.55f, 1.0f);

    return plant;
}
