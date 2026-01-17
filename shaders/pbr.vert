#version 450

// Uniform 缓冲区 - Set 0: Camera
layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
} camera;

// Set 1, Binding 0: Object Transform
layout(set = 1, binding = 0) uniform ObjectBuffer {
    mat4 model;
    mat4 normalMatrix;
} transform;

// 输入属性
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// 输出到片段着色器
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    // 计算世界空间位置
    vec4 worldPos = transform.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;

    // 传递法线（世界空间，使用法线矩阵正确变换）
    fragNormal = mat3(transform.normalMatrix) * inNormal;

    // 传递纹理坐标
    fragTexCoord = inTexCoord;

    // 输出裁剪空间位置
    gl_Position = camera.projection * camera.view * worldPos;
}
