#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// 输出到片元着色器的插值数据
out vec3 WorldPos;      // 世界空间位置
out vec3 Normal;        // 世界空间法线
out vec2 TexCoords;     // 纹理坐标（为以后贴图留接口）
out vec4 FragPosLightSpace;  // 光源空间位置（用于阴影采样）

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;  // 光源空间矩阵（用于阴影）

void main()
{
    // 顶点位置变换到世界空间
    WorldPos = vec3(model * vec4(aPos, 1.0));

    // 法线按模型矩阵的逆转置变换到世界空间
    Normal = mat3(transpose(inverse(model))) * aNormal;

    TexCoords = aTexCoords;

    // 计算光源空间位置（用于阴影采样）
    FragPosLightSpace = lightSpaceMatrix * vec4(WorldPos, 1.0);

    // 最终裁剪空间位置
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}