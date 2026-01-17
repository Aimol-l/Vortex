#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <array>

/**
 * @brief 光源类型
 */
enum class LightType {
    Directional, // 方向光（如太阳光）
    Point,       // 点光源（灯泡）
    Spot,        // 聚光灯（手电筒）
    Area         // 面光源（未来扩展）
};

/**
 * @brief 光源数据（用于 UBO 或 Push Constants）
 *
 * 这个结构体直接对应着色器中的光源定义，
 * 可以直接上传到 GPU。
 */
struct LightData {
    alignas(16) glm::vec3 position;      // 位置（点光源和聚光灯）
    alignas(4)  LightType type;          // 光源类型

    alignas(16) glm::vec3 direction;     // 方向（方向光和聚光灯）
    alignas(4)  float intensity;         // 强度

    alignas(16) glm::vec3 color;         // 光照颜色
    alignas(4)  float radius;            // 影响半径（点光源和聚光灯）

    // 聚光灯参数
    alignas(4)  float innerConeAngle;    // 内锥角（度）
    alignas(4)  float outerConeAngle;    // 外锥角（度）

    // 衰减参数（点光源）
    alignas(4)  float constant;          // 常数衰减
    alignas(4)  float linear;            // 线性衰减
    alignas(4)  float quadratic;         // 二次衰减

    // 阴影参数
    alignas(4)  bool castShadows;        // 是否投射阴影
    alignas(4)  int shadowMapIndex;      // 阴影贴图索引
};

/**
 * @class Light
 * @brief 光源对象（C++ 侧使用）
 *
 * Light 封装了光源的所有属性，提供：
 * - 类型安全的光源参数设置
 * - 自动更新 GPU 数据
 * - 阴影贴图管理
 * - 光源视锥体计算（用于阴影映射）
 *
 * 使用场景：
 * - 场景照明
 * - 延迟渲染的光照阶段
 * - 阴影贴图生成
 */
class Light {
public:
    explicit Light(LightType type = LightType::Point);
    ~Light() = default;

    // 禁止拷贝，允许移动
    Light(const Light&) = delete;
    Light& operator=(const Light&) = delete;
    Light(Light&&) = default;
    Light& operator=(Light&&) = default;

    // --- 光源类型 ---

    LightType getType() const { return m_type; }
    void setType(LightType type) { m_type = type; m_isDirty = true; }

    // --- 基本属性 ---

    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; m_isDirty = true; }

    const glm::vec3& getDirection() const { return m_direction; }
    void setDirection(const glm::vec3& direction) { m_direction = glm::normalize(direction); m_isDirty = true; }

    const glm::vec3& getColor() const { return m_color; }
    void setColor(const glm::vec3& color) { m_color = color; m_isDirty = true; }

    float getIntensity() const { return m_intensity; }
    void setIntensity(float intensity) { m_intensity = intensity; m_isDirty = true; }

    // --- 点光源和聚光灯 ---

    float getRadius() const { return m_radius; }
    void setRadius(float radius) { m_radius = radius; m_isDirty = true; }

    // --- 聚光灯 ---

    float getInnerConeAngle() const { return m_innerConeAngle; }
    void setInnerConeAngle(float angle) { m_innerConeAngle = angle; m_isDirty = true; }

    float getOuterConeAngle() const { return m_outerConeAngle; }
    void setOuterConeAngle(float angle) { m_outerConeAngle = angle; m_isDirty = true; }

    // --- 衰减参数 ---

    void setAttenuation(float constant, float linear, float quadratic) {
        m_constant = constant;
        m_linear = linear;
        m_quadratic = quadratic;
        m_isDirty = true;
    }

    float getConstantAttenuation() const { return m_constant; }
    float getLinearAttenuation() const { return m_linear; }
    float getQuadraticAttenuation() const { return m_quadratic; }

    /**
     * @brief 设置物理准确的衰减（基于距离）
     * @param range 影响范围
     */
    void setPhysicalAttenuation(float range);

    // --- 阴影 ---

    bool castsShadows() const { return m_castShadows; }
    void setCastShadows(bool cast) { m_castShadows = cast; m_isDirty = true; }

    int getShadowMapIndex() const { return m_shadowMapIndex; }
    void setShadowMapIndex(int index) { m_shadowMapIndex = index; m_isDirty = true; }

    /**
     * @brief 获取光源的投影矩阵（用于阴影贴图）
     * @param aspect 宽高比
     * @param near 近平面
     * @param far 远平面
     */
    glm::mat4 getProjectionMatrix(float aspect = 1.0f, float near = 0.1f, float far = 100.0f) const;

    /**
     * @brief 获取光源的视图矩阵（用于阴影贴图）
     */
    glm::mat4 getViewMatrix() const;

    // --- 数据获取 ---

    /**
     * @brief 获取 LightData（上传到 GPU）
     */
    LightData getLightData() const;

    /**
     * @brief 是否数据已改变
     */
    bool isDirty() const { return m_isDirty; }

    /**
     * @brief 清除脏标记
     */
    void clearDirty() { m_isDirty = false; }

    // --- 工厂方法 ---

    /**
     * @brief 创建方向光（如太阳）
     */
    static std::shared_ptr<Light> createDirectional(
        const glm::vec3& direction = glm::vec3(0.0f, -1.0f, 0.0f),
        const glm::vec3& color = glm::vec3(1.0f),
        float intensity = 1.0f);

    /**
     * @brief 创建点光源（如灯泡）
     */
    static std::shared_ptr<Light> createPoint(
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& color = glm::vec3(1.0f),
        float intensity = 1.0f,
        float radius = 10.0f);

    /**
     * @brief 创建聚光灯（如手电筒）
     */
    static std::shared_ptr<Light> createSpot(
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& direction = glm::vec3(0.0f, -1.0f, 0.0f),
        const glm::vec3& color = glm::vec3(1.0f),
        float intensity = 1.0f,
        float radius = 10.0f,
        float innerAngle = 12.5f,
        float outerAngle = 17.5f);

private:
    // 光源属性
    LightType m_type;
    glm::vec3 m_position{0.0f};
    glm::vec3 m_direction{0.0f, -1.0f, 0.0f};
    glm::vec3 m_color{1.0f};
    float m_intensity = 1.0f;

    // 点光源和聚光灯
    float m_radius = 10.0f;
    float m_constant = 1.0f;
    float m_linear = 0.09f;
    float m_quadratic = 0.032f;

    // 聚光灯
    float m_innerConeAngle = 12.5f;
    float m_outerConeAngle = 17.5f;

    // 阴影
    bool m_castShadows = false;
    int m_shadowMapIndex = -1;

    // 脏标记
    bool m_isDirty = true;
};

/**
 * @brief 光源 Uniform Buffer 数据
 *
 * 包含场景中最多支持的光源数量
 */
struct LightUniforms {
    static constexpr int MAX_LIGHTS = 16; // 最多 16 个光源

    alignas(4)  int lightCount = 0;                     // 当前光源数量
    alignas(16) glm::vec3 ambientColor = glm::vec3(0.1f); // 环境光
    alignas(16) LightData lights[MAX_LIGHTS];          // 光源数组
};

/**
 * @brief 级联阴影贴图（Cascaded Shadow Maps）
 *
 * 用于方向光的大范围阴影（如户外场景）
 */
class CascadedShadowMaps {
public:
    explicit CascadedShadowMaps(int cascades = 4);
    ~CascadedShadowMaps() = default;

    /**
     * @brief 更新级联分割
     * @param near 近平面
     * @param far 远平面
     * @param splits 分割距离
     */
    void updateCascades(float near, float far, const std::vector<float>& splits);

    /**
     * @brief 获取级联的视图-投影矩阵
     */
    const std::vector<glm::mat4>& getVPs() const { return m_vps; }

    /**
     * @brief 获取级联的分割距离
     */
    const std::vector<float>& getSplits() const { return m_splits; }

private:
    int m_cascadeCount;
    std::vector<glm::mat4> m_vps;
    std::vector<float> m_splits;
};

/**
 * @brief 光照贴图（Light Probes，未来扩展）
 *
 * 用于全局光照和间接光照
 */
class LightProbe {
public:
    glm::vec3 position;
    float radius;

    // 球谐系数（ irradiance）
    std::array<glm::vec3, 9> sphericalHarmonics;

    // 预过滤环境贴图（反射）
    std::array<glm::vec3, 6> prefilteredEnv;
};
