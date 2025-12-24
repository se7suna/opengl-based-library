#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 Tangent;       // 世界空间切线（用于法线贴图）
in vec4 FragPosLightSpace[8];  // 点光源空间位置（用于阴影采样）
in vec4 FragPosDirLightSpace;  // 方向光光源空间位置（用于阴影采样）

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
uniform sampler2D shadowMap[8];  // 每个点光源的阴影贴图
uniform bool shadowsEnabled[8];  // 每个点光源是否启用阴影

// ===== 点光源定义（支持多个灯光） =====
struct PointLight {
    vec3 position;   // 世界空间位置
    vec3 color;      // 光源颜色（强度编码在 color 和 intensity 里）
    float intensity; // 光强（标量）
};

// 预留 8 个点光源位，实际使用数量由 lightCount 控制
uniform PointLight lights[8];
uniform int lightCount;  // 当前启用的点光源数量

// ===== 方向光定义（用于模拟自然光） =====
struct DirectionalLight {
    vec3 direction;   // 光线方向（从光源指向目标）
    vec3 color;       // 光源颜色
    float intensity;  // 光强（标量）
};

uniform DirectionalLight dirLight;  // 方向光（自然光）
uniform bool dirLightEnabled;       // 方向光是否启用
uniform sampler2D dirLightShadowMap;  // 方向光阴影贴图
uniform bool dirLightShadowsEnabled;   // 方向光阴影是否启用
uniform mat4 dirLightSpaceMatrix;     // 方向光光源空间矩阵

// 观察者位置（世界空间）
uniform vec3 camPos;

// 材质选项：是否使用GLOSS贴图（需要反转roughness）
uniform bool useGlossMap;  // true表示roughnessMap实际上是GLOSS贴图，需要反转

// 材质参数（保留用于兼容性，但设置为白色不进行调制）
uniform vec3 materialAlbedo;  // 设置为(1,1,1)，不进行任何颜色调制

// 是否使用triplanar mapping（某些对象如地板、墙壁可能需要禁用）
uniform bool useTriplanarMapping;  // true使用triplanar，false使用传统UV

const float PI = 3.14159265359;

// ===== 阴影相关函数 =====

// 计算点光源阴影因子（PCF软阴影）
float ShadowCalculation(int lightIndex, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    if (!shadowsEnabled[lightIndex]) {
        return 1.0;  // 如果阴影未启用，返回1.0（无阴影）
    }
    
    // 执行透视除法，将裁剪空间坐标转换为NDC坐标 [-1,1]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // 转换到 [0,1] 范围
    projCoords = projCoords * 0.5 + 0.5;
    
    // 检查是否在阴影贴图范围内
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;  // 超出范围，不在阴影中
    }
    
    // 当前片元的深度值
    float currentDepth = projCoords.z;
    
    // 偏移量（避免阴影失真）
    // 减小bias，让阴影更明显
    float bias = max(0.02 * (1.0 - dot(normal, lightDir)), 0.001);
    
    // PCF软阴影（3x3采样）
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap[lightIndex], 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap[lightIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return 1.0 - shadow;
}

// 计算方向光阴影因子（PCF软阴影）
float DirLightShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    if (!dirLightShadowsEnabled) {
        return 1.0;  // 如果阴影未启用，返回1.0（无阴影）
    }
    
    // 执行透视除法，将裁剪空间坐标转换为NDC坐标 [-1,1]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // 转换到 [0,1] 范围
    projCoords = projCoords * 0.5 + 0.5;
    
    // 检查是否在阴影贴图范围内
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;  // 超出范围，不在阴影中
    }
    
    // 当前片元的深度值
    float currentDepth = projCoords.z;
    
    // 偏移量（避免阴影失真）
    // 减小bias，让阴影更明显
    float bias = max(0.02 * (1.0 - dot(normal, lightDir)), 0.001);
    
    // PCF软阴影（3x3采样）
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(dirLightShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(dirLightShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return 1.0 - shadow;
}

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

// Triplanar mapping函数
vec3 triplanarSample(sampler2D tex, vec3 pos, vec3 normal, float scale) {
    // 计算三个平面的权重（基于法线）
    vec3 blend = abs(normal);
    blend = normalize(max(blend, 0.00001)); // 避免除零
    float b = (blend.x + blend.y + blend.z);
    blend /= vec3(b, b, b);
    
    // 采样三个平面
    vec3 x = pow(texture(tex, pos.yz * scale).rgb, vec3(2.2));
    vec3 y = pow(texture(tex, pos.xz * scale).rgb, vec3(2.2));
    vec3 z = pow(texture(tex, pos.xy * scale).rgb, vec3(2.2));
    
    // 混合三个平面的采样结果
    return x * blend.x + y * blend.y + z * blend.z;
}

float triplanarSampleFloat(sampler2D tex, vec3 pos, vec3 normal, float scale) {
    vec3 blend = abs(normal);
    blend = normalize(max(blend, 0.00001));
    float b = (blend.x + blend.y + blend.z);
    blend /= vec3(b, b, b);
    
    float x = texture(tex, pos.yz * scale).r;
    float y = texture(tex, pos.xz * scale).r;
    float z = texture(tex, pos.xy * scale).r;
    
    return x * blend.x + y * blend.y + z * blend.z;
}

void main()
{
    // ===== 根据设置选择使用Triplanar Mapping或传统UV =====
    float texScale = 0.5; // 纹理缩放，可以根据需要调整
    vec3  albedo;
    float metallic;
    float roughness;
    float ao;
    
    if (useTriplanarMapping) {
        // 使用Triplanar Mapping从贴图中采样 PBR 材质参数
        // 使用世界坐标和法线进行triplanar采样，解决贴图方向问题
        albedo    = triplanarSample(albedoMap, WorldPos, Normal, texScale);
        metallic  = triplanarSampleFloat(metallicMap, WorldPos, Normal, texScale);
        roughness = triplanarSampleFloat(roughnessMap, WorldPos, Normal, texScale);
        ao        = triplanarSampleFloat(aoMap, WorldPos, Normal, texScale);
    } else {
        // 使用传统UV坐标采样（适用于地板、墙壁等需要正确UV映射的对象）
        albedo    = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
        metallic  = texture(metallicMap, TexCoords).r;
        roughness = texture(roughnessMap, TexCoords).r;
        ao        = texture(aoMap, TexCoords).r;
    }
    
    // 如果使用GLOSS贴图，需要反转（GLOSS = 1 - Roughness）
    if (useGlossMap) {
        roughness = 1.0 - roughness;
    }
    
    // 纯PBR渲染：不进行MTL颜色调制
    // materialAlbedo应该始终为(1,1,1)，所以这里不进行乘法运算
    // albedo保持从贴图采样的原始值

    // ===== 使用法线贴图（TBN矩阵） =====
    vec3 N = normalize(Normal);
    
    // 只有在非triplanar模式下才使用法线贴图（triplanar模式使用几何法线）
    if (!useTriplanarMapping) {
        // 构建TBN矩阵（Tangent-Bitangent-Normal）
        vec3 T = normalize(Tangent);
        // 确保切线垂直于法线（Gram-Schmidt正交化）
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T);  // 副切线（Bitangent）
        
        // 构建TBN矩阵（从切线空间到世界空间）
        mat3 TBN = mat3(T, B, N);
        
        // 从法线贴图采样切线空间法线
        vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;  // 从[0,1]转换到[-1,1]
        
        // 将切线空间法线转换到世界空间
        N = normalize(TBN * tangentNormal);
    }
    // triplanar模式下使用几何法线（N已经normalize）
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

        // 计算阴影因子
        float shadow = ShadowCalculation(i, FragPosLightSpace[i], N, L);
        
        // 最终每盏灯的贡献（应用阴影）
        vec3 diffuseBRDF = kD * albedo / PI;
        vec3 contribution = (diffuseBRDF + specular) * radiance * NdotL * shadow;

        Lo += contribution;
    }
    
    // ===== 方向光（自然光）光照计算 =====
    if (dirLightEnabled) {
        // 方向光方向（从光源指向目标，需要取反）
        vec3 L = normalize(-dirLight.direction);
        vec3 H = normalize(V + L);
        
        // 方向光没有距离衰减，直接使用强度和颜色
        // 方向光通常比点光源更强，因为它是模拟太阳光
        vec3 radiance = dirLight.color * dirLight.intensity;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, rough);
        float G   = GeometrySmith(N, V, L, rough);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= (1.0 - metallic);
        
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        
        vec3 numerator   = NDF * G * F;
        float denom      = 4.0 * max(NdotV * NdotL, 0.0001);
        vec3 specular    = numerator / denom;
        
        // 计算方向光阴影因子
        float dirShadow = DirLightShadowCalculation(FragPosDirLightSpace, N, L);
        
        // 方向光贡献（应用阴影）
        vec3 diffuseBRDF = kD * albedo / PI;
        vec3 dirContribution = (diffuseBRDF + specular) * radiance * NdotL * dirShadow;
        
        Lo += dirContribution;
    }

    // 简单的环境光（未使用 IBL，仅常数环境 + AO）
    vec3 ambient = vec3(0.15) * albedo * ao;

    vec3 color = ambient + Lo;

    // Reinhard 色调映射 + gamma 校正
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}