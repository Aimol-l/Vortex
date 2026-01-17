#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

/**
 * @class Transform
 * @brief 表示物体在 3D 空间中的位置、旋转和缩放
 *
 * Transform 是场景图的核心组件，支持：
 * - 层级关系：父-子关系，子物体的变换相对于父物体
 * - 多种旋转表示：欧拉角、四元数、旋转矩阵
 * - 变换矩阵缓存：避免重复计算，优化性能
 * - 脏标记系统：只在变换改变时重新计算矩阵
 *
 * 使用建议：
 * - 对于动画、骨骼等需要旋转插值的情况，使用四元数
 * - 对于简单的旋转，欧拉角更直观
 * - 避免频繁修改变换，尽量批量更新
 */
class Transform {
public:
    Transform();
    ~Transform() = default;

    // 禁止拷贝，允许移动
    Transform(const Transform&) = delete;
    Transform& operator=(const Transform&) = delete;
    Transform(Transform&&) = default;
    Transform& operator=(Transform&&) = default;

    // --- 父子关系 ---

    /**
     * @brief 设置父 Transform
     * @param parent 父节点指针，nullptr 表示根节点
     * @note 设置父节点后，本节点的变换将相对于父节点
     */
    void setParent(Transform* parent);

    /**
     * @brief 添加子节点
     * @param child 子节点指针
     * @note 会自动设置 child 的父节点为 this
     */
    void addChild(Transform* child);

    /**
     * @brief 移除子节点
     * @param child 要移除的子节点
     */
    void removeChild(Transform* child);

    Transform* getParent() const { return m_parent; }
    const std::vector<Transform*>& getChildren() const { return m_children; }

    // --- 位置 ---

    /**
     * @brief 设置本地位置（相对于父节点）
     */
    void setPosition(const glm::vec3& position);

    /**
     * @brief 设置世界位置（绝对位置）
     * @note 会重新计算本地位置，需要遍历父链
     */
    void setWorldPosition(const glm::vec3& position);

    const glm::vec3& getPosition() const { return m_position; }

    /**
     * @brief 获取世界位置
     * @note 会计算完整的变换链
     */
    glm::vec3 getWorldPosition() const;

    /**
     * @brief 平移（本地空间）
     * @param delta 平移向量
     */
    void translate(const glm::vec3& delta);

    // --- 旋转 ---

    // 欧拉角（度）
    void setRotation(const glm::vec3& eulerAngles); // (pitch, yaw, roll)
    void setRotation(float pitch, float yaw, float roll);
    glm::vec3 getRotation() const { return m_eulerAngles; }

    // 四元数（推荐用于动画）
    void setQuaternion(const glm::quat& quaternion);
    const glm::quat& getQuaternion() const { return m_quaternion; }

    // 旋转矩阵
    void setRotationMatrix(const glm::mat3& rotationMatrix);
    glm::mat3 getRotationMatrix() const;

    /**
     * @brief 绕轴旋转（本地空间）
     * @param axis 旋转轴（建议归一化）
     * @param angle 旋转角度（度）
     */
    void rotate(const glm::vec3& axis, float angle);

    /**
     * @brief 绕世界坐标轴旋转
     * @param axis 世界坐标轴
     * @param angle 旋转角度（度）
     */
    void rotateWorld(const glm::vec3& axis, float angle);

    // --- 缩放 ---

    /**
     * @brief 设置均匀缩放
     */
    void setScale(float scale);

    /**
     * @brief 设置非均匀缩放
     */
    void setScale(const glm::vec3& scale);

    const glm::vec3& getScale() const { return m_scale; }

    // --- 变换矩阵 ---

    /**
     * @brief 获取本地变换矩阵（相对于父节点）
     * @return mat4 本地模型矩阵
     * @note 包含 T * R * S 变换
     */
    const glm::mat4& getLocalMatrix() const;

    /**
     * @brief 获取世界变换矩阵（绝对变换）
     * @return mat4 世界模型矩阵
     * @note 会累加所有父节点的变换
     */
    glm::mat4 getWorldMatrix() const;

    /**
     * @brief 获取法线变换矩阵
     * @return mat3 用于变换法线向量
     * @note 法线需要用 (M^-1)^T 变换，不能用普通模型矩阵
     */
    glm::mat3 getNormalMatrix() const;

    // --- 方向向量（本地空间）---

    /**
     * @brief 获取前方向（本地 -Z 方向）
     */
    glm::vec3 getForward() const;

    /**
     * @brief 获取右方向（本地 +X 方向）
     */
    glm::vec3 getRight() const;

    /**
     * @brief 获取上方向（本地 +Y 方向）
     */
    glm::vec3 getUp() const;

    // --- 脏标记系统 ---

    /**
     * @brief 标记为脏（强制重新计算矩阵）
     * @param recursive 是否递归标记所有子节点
     */
    void setDirty(bool recursive = false);

    /**
     * @brief 强制重新计算矩阵（即使没有脏标记）
     */
    void forceUpdate();

    bool isDirty() const { return m_isDirty; }

    // --- 实用工具 ---

    /**
     * @brief 看向目标点（本地空间）
     * @param target 目标位置
     * @param up 上方向参考向量
     */
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    /**
     * @brief 对齐到上方向向量
     * @param up 目标上方向
     */
    void alignToUp(const glm::vec3& up);

    /**
     * @brief 重置为单位变换
     */
    void reset();

private:
    /**
     * @brief 从内部数据重新计算本地矩阵
     */
    void updateLocalMatrix() const;

    /**
     * @brief 将欧拉角转换为四元数
     */
    void updateQuaternionFromEuler();

    /**
     * @brief 将四元数转换为欧拉角
     */
    void updateEulerFromQuaternion();

    // 父子关系
    Transform* m_parent = nullptr;
    std::vector<Transform*> m_children;

    // 本地变换数据
    glm::vec3 m_position{0.0f};
    glm::vec3 m_scale{1.0f};
    glm::quat m_quaternion{1.0f, 0.0f, 0.0f, 0.0f}; // 四元数 (w, x, y, z)
    glm::vec3 m_eulerAngles{0.0f};                   // 欧拉角 (度)

    // 缓存的矩阵
    mutable glm::mat4 m_localMatrix{1.0f};           // 本地变换矩阵
    mutable bool m_isDirty = true;                   // 脏标记
};
