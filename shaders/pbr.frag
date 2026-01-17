#version 450

// 来自顶点着色器的输入
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

// Camera buffer from set 0 (shared with vertex shader)
layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
} camera;

// Set 1, Binding 1: Light buffer
layout(set = 1, binding = 1) uniform LightBuffer {
    vec3 position;
    float intensity;
    vec3 color;
    uint type;
} light;

// Set 1, Binding 2: Material buffer
layout(set = 1, binding = 2) uniform MaterialBuffer {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
} material;

// 纹理采样器 - Set 1, Bindings 3-6
layout(set = 1, binding = 3) uniform sampler2D albedoMap;
layout(set = 1, binding = 4) uniform sampler2D normalMap;
layout(set = 1, binding = 5) uniform sampler2D metallicMap;
layout(set = 1, binding = 6) uniform sampler2D roughnessMap;

// 输出颜色
layout(location = 0) out vec4 outColor;

// 常量
const float PI = 3.14159265359;

// === PBR 辅助函数 ===

// 1. 法线分布函数 (Trowbridge-Reitz GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// 2. 几何函数 (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 3. 菲涅尔方程 (Schlick近似)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 带粗糙度的菲涅尔（用于环境光照）
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // 1. 准备材质参数（从纹理或UBO获取）
    vec3 albedo = texture(albedoMap, fragTexCoord).rgb;
    float metallic = texture(metallicMap, fragTexCoord).r;
    float roughness = texture(roughnessMap, fragTexCoord).r;

    // 如果纹理是纯黑或无效，使用UBO值作为后备
    if (metallic < 0.01) metallic = material.metallic;
    if (roughness < 0.01) roughness = material.roughness;

    float ao = material.ao;

    // 2. 归一化输入向量
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camera.cameraPos - fragPos);
    vec3 L = normalize(light.position - fragPos);
    vec3 H = normalize(V + L);

    // 3. 计算反射率（F0）
    // 对于电介质（非金属）使用0.04，金属使用albedo
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // 4. === Cook-Torrance BRDF ===

    // 4.1 法线分布
    float NDF = DistributionGGX(N, H, roughness);

    // 4.2 几何遮蔽
    float G = GeometrySmith(N, V, L, roughness);

    // 4.3 菲涅尔
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // 4.4 计算镜面反射
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // 4.5 计算漫反射和能量守恒
    vec3 kS = F; // 镜面反射比率
    vec3 kD = vec3(1.0) - kS; // 漫反射比率
    kD *= 1.0 - metallic; // 金属没有漫反射

    // 4.6 从辐照度获取漫反射光照（简化版本，直接用方向光）
    vec3 irradiance = vec3(1.0); // 简化：假设均匀辐照度
    vec3 diffuse = kD * albedo * irradiance;

    // 4.7 计算最终光照
    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = light.color * light.intensity;
    vec3 Lo = (diffuse + specular) * radiance * NdotL;

    // 5. 环境光照（简化版本）
    // 完整实现应该使用IBL（基于图像的照明）
    vec3 R = reflect(-V, N);
    vec3 F_ambient = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS_ambient = F_ambient;
    vec3 kD_ambient = (1.0 - kS_ambient) * (1.0 - metallic);
    vec3 irradiance_ambient = vec3(0.03) * albedo; // 简化：弱的环境光
    vec3 ambient = kD_ambient * irradiance_ambient * ao;

    // 6. 最终颜色 = 环境光 + 直接光照
    vec3 color = ambient + Lo;

    // 7. HDR色调映射（简单的Reinhard）
    color = color / (color + vec3(1.0));

    // 8. Gamma校正
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
