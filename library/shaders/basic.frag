#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

// 材质结构体：仅保留CPU传递的3个参数（无ambient）
struct Material {
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;

// 两个方向光数组（与CPU一致）
struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
};
uniform DirLight dirLights[2];

uniform vec3 viewPos;

void main() {
    vec3 norm = normalize(Normal);
    vec3 totalLight = vec3(0.0f);  // 累计所有光源贡献

    // 遍历两个方向光
    for (int i = 0; i < 2; i++) {
        // 1. 环境光（每个光源独立环境光，避免全黑）
        vec3 ambient = dirLights[i].color * 0.15f;  // 环境光强度15%（降低，突出漫反射差异）

        // 2. 漫反射（核心：法线与光线夹角决定亮度）
        vec3 lightDir = normalize(-dirLights[i].direction);
        float diffFactor = max(dot(norm, lightDir), 0.0f);  // 0~1，夹角越小越亮
        vec3 diffuse = dirLights[i].intensity * dirLights[i].color * diffFactor * material.diffuse;

        // 3. 镜面反射（高光，增加层次感）
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float specFactor = pow(max(dot(norm, halfwayDir), 0.0f), material.shininess);
        vec3 specular = dirLights[i].intensity * dirLights[i].color * specFactor * material.specular;

        // 累加当前光源的光照
        totalLight += ambient + diffuse + specular;
    }

    FragColor = vec4(totalLight, 1.0f);
}