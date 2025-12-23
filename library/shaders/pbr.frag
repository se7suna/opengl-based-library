#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;

// ===== PBR 材质贴图（Metallic-Roughness 工作流） =====
// 这些 sampler2D 会在 C++ 中绑定：
//  albedoMap    → BaseColor.jpg  （sRGB 纹理）
//  normalMap    → Normal.png     （tangent-space normal，当前暂时不用 TBN，仅做占位）
//  metallicMap  → Metallic.jpg   （R 通道）
//  roughnessMap → Roughness.jpg  （R 通道）
//  aoMap        → AmbientOcclusion.jpg （R 通道）
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// ===== 点光源定义（支持多个灯光） =====
struct PointLight {
    vec3 position;   // 世界空间位置
    vec3 color;      // 光源颜色（强度编码在 color 和 intensity 里）
    float intensity; // 光强（标量）
};

// 预留 8 个点光源位，实际使用数量由 lightCount 控制
uniform PointLight lights[8];
uniform int lightCount;  // 当前启用的点光源数量

// 观察者位置（世界空间）
uniform vec3 camPos;

// 材质选项：是否使用GLOSS贴图（需要反转roughness）
uniform bool useGlossMap;  // true表示roughnessMap实际上是GLOSS贴图，需要反转

const float PI = 3.14159265359;

// ===== Cook-Torrance BRDF 相关函数 =====

// 法线分布函数 NDF: Trowbridge-Reitz GGX
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;

    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0001);
}

// 几何项 G: Schlick-GGX for one direction
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;  // 基于 direct lighting 的经验公式

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0001);
}

// 几何项 G: Smith 方法，视线和光线同时考虑
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggxV  = GeometrySchlickGGX(NdotV, roughness);
    float ggxL  = GeometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

// 菲涅尔项 F: Schlick 近似
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // ===== 从贴图中采样 PBR 材质参数 =====
    // 颜色贴图是 sRGB，需要转到线性空间
    vec3  albedo    = pow(texture(albedoMap,    TexCoords).rgb, vec3(2.2));
    float metallic  = texture(metallicMap,      TexCoords).r;
    float roughness = texture(roughnessMap,     TexCoords).r;
    // 如果使用GLOSS贴图，需要反转（GLOSS = 1 - Roughness）
    if (useGlossMap) {
        roughness = 1.0 - roughness;
    }
    float ao        = texture(aoMap,            TexCoords).r;

    // TODO: 以后引入 TBN 矩阵后再真正使用 normal map
    // vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    // 先使用几何法线，法线贴图留到后续扩展
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    // 基础反射率 F0：非金属通常是 0.04，金属用 albedo 代替
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // 累积光照
    vec3 Lo = vec3(0.0);

    // 防止粗糙度为 0 导致 NDF 发散
    float rough = clamp(roughness, 0.04, 1.0);

    for (int i = 0; i < lightCount; ++i)
    {
        // 光照方向
        vec3 L = normalize(lights[i].position - WorldPos);
        vec3 H = normalize(V + L);

        // 距离衰减（平方反比）
        float distance    = length(lights[i].position - WorldPos);
        float attenuation = 1.0 / max(distance * distance, 0.01);
        vec3 radiance     = lights[i].color * lights[i].intensity * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, rough);
        float G   = GeometrySmith(N, V, L, rough);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;                        // 镜面部分
        vec3 kD = vec3(1.0) - kS;           // 漫反射部分
        kD *= (1.0 - metallic);             // 金属几乎没有漫反射

        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);

        vec3 numerator   = NDF * G * F;
        float denom      = 4.0 * max(NdotV * NdotL, 0.0001);
        vec3 specular    = numerator / denom;

        // 最终每盏灯的贡献
        vec3 diffuseBRDF = kD * albedo / PI;
        vec3 contribution = (diffuseBRDF + specular) * radiance * NdotL;

        Lo += contribution;
    }

    // 简单的环境光（未使用 IBL，仅常数环境 + AO）
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    // Reinhard 色调映射 + gamma 校正
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}