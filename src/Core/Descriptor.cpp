#include "Core/Descriptor.h"
#include "Core/Context.h"
#include "Assets/Material.h"
#include "Scene/UniformBuffer.h"
#include <print>
#include <stdexcept>

// ---------------------------------------------------------------------------
// 辅助模板递归函数：在编译时生成 vk::DescriptorSetLayoutBinding
// ---------------------------------------------------------------------------
// 此函数接收一个 binding 索引和一个用于存储结果的 vector 引用。
// 它为模板类型 T 创建一个 binding，然后递归处理剩余的类型。
template <typename T, typename... Rest>
void generateBindings(uint32_t binding, std::vector<vk::DescriptorSetLayoutBinding>& bindings) {
    static_assert(DescriptorTraits<T>::IsValid, "Failed to generate bindings for an unknown type. Please ensure a DescriptorTraits specialization exists.");

    vk::DescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = DescriptorTraits<T>::Type;
    layoutBinding.descriptorCount = 1; // 每个类型默认只有一个描述符
    layoutBinding.stageFlags = DescriptorTraits<T>::Stages;
    bindings.push_back(layoutBinding);

    // 如果还有更多类型，递归处理，并将 binding 索引加一
    if constexpr (sizeof...(Rest) > 0) {
        generateBindings<Rest...>(binding + 1, bindings);
    }
}
// ---------------------------------------------------------------------------
// 布局创建 (模板函数)
// ---------------------------------------------------------------------------
template<typename... Types>
void DescriptorManager::createLayout(uint32_t setIndex, uint32_t bindStart) {
    if (m_layouts.count(setIndex)) {
        throw std::runtime_error("Layout for set " + std::to_string(setIndex) + " has already been created.");
    }

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    generateBindings<Types...>(bindStart, bindings);

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    m_layouts[setIndex] = m_context->getDevice().createDescriptorSetLayout(layoutInfo);

    std::println("Created DescriptorSetLayout for set {} with {} bindings", setIndex, bindings.size());
}
// ---------------------------------------------------------------------------
// DescriptorManager 类实现
// ---------------------------------------------------------------------------

DescriptorManager::DescriptorManager(Context* context)
    : m_context(context) {
    std::println("DescriptorManager created");
}

DescriptorManager::~DescriptorManager() {
    std::println("DescriptorManager destroying...");

    auto device = m_context->getDevice();

    // 首先销毁描述符池，这会释放所有从中分配的描述符集
    if (m_descriptorPool) {
        device.destroyDescriptorPool(m_descriptorPool);
        m_descriptorPool = nullptr;
    }

    // 清空描述符集容器
    m_sets.clear();

    // 然后销毁所有描述符集布局
    for (auto& pair : m_layouts) {
        if (pair.second) {
            device.destroyDescriptorSetLayout(pair.second);
        }
    }
    m_layouts.clear();

    std::println("DescriptorManager destroyed");
}



// ---------------------------------------------------------------------------
// 描述符池创建
// ---------------------------------------------------------------------------
void DescriptorManager::createPool(const std::unordered_map<uint32_t, uint32_t>& setCapacity) {
    std::unordered_map<vk::DescriptorType, uint32_t> poolSizeCounts;
    uint32_t totalSets = 0;
    // 遍历每个 set 的容量需求，计算池中每种描述符类型的总数
    for (const auto& [setIndex, count] : setCapacity) {
        totalSets += count;
        // 检查该 set 的布局是否存在
        if (!m_layouts.count(setIndex)) {
            throw std::runtime_error("Capacity requested for set " + std::to_string(setIndex) + ", but its layout was not created.");
        }
        // --- 这里需要根据你的具体布局来手动计算 ---
        if (setIndex == 0) {        // Set 0: Camera
            poolSizeCounts[vk::DescriptorType::eUniformBuffer] += count;
        } else if (setIndex == 1) { // Set 1: Object (Transform, Light, Material, 4 Textures)
            poolSizeCounts[vk::DescriptorType::eUniformBuffer] += count * 3; // 3 UBOs
            poolSizeCounts[vk::DescriptorType::eCombinedImageSampler] += count * 4; // 4 Samplers
        }
    }
    // 填充 vk::DescriptorPoolSize 数组
    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (const auto& [type, count] : poolSizeCounts) {
        if (count > 0) {
            poolSizes.push_back({type, count});
        }
    }
    if (poolSizes.empty()) {
        std::println("Warning: No descriptors needed for the pool. Skipping pool creation.");
        return;
    }

    auto device = m_context->getDevice();

    // 创建描述符池
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = totalSets;

    try {
        m_descriptorPool = device.createDescriptorPool(poolInfo);
        std::println("Successfully created descriptor pool with max sets: {}", totalSets);
    } catch (const vk::SystemError& err) {
        throw std::runtime_error("Failed to create descriptor pool: " + std::string(err.what()));
    }
}


// ---------------------------------------------------------------------------
// 描述符集分配
// ---------------------------------------------------------------------------
void DescriptorManager::allocateAllSets(const std::unordered_map<uint32_t, uint32_t>& setCapacity) {
    if (!m_descriptorPool) {
        throw std::runtime_error("Cannot allocate sets because the descriptor pool has not been created.");
    }
    // 遍历每个需要分配的 set
    for (const auto& [setIndex, count] : setCapacity) {
        auto layoutIt = m_layouts.find(setIndex);
        if (layoutIt == m_layouts.end()) {
            continue;
        }
        // 为当前 set 的所有实例准备一个包含相同布局的 vector
        std::vector<vk::DescriptorSetLayout> layouts(count, layoutIt->second);
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = count;
        allocInfo.pSetLayouts = layouts.data();
        // 分配并存储结果
        m_sets[setIndex] = m_context->getDevice().allocateDescriptorSets(allocInfo);
        std::println("Allocated {} descriptor sets for set {}", count, setIndex);
    }
}

vk::DescriptorSetLayout DescriptorManager::getLayout(uint32_t setIndex) const {
    auto it = m_layouts.find(setIndex);
    if (it == m_layouts.end()) {
        throw std::runtime_error("Layout for set " + std::to_string(setIndex) + " not found.");
    }
    return it->second;
}

vk::DescriptorSet DescriptorManager::getSet(uint32_t setIndex, uint32_t instanceIndex) const {
    auto setIt = m_sets.find(setIndex);
    if (setIt == m_sets.end()) {
        throw std::runtime_error("Sets for set " + std::to_string(setIndex) + " have not been allocated. Call allocateAllSets() first.");
    }
    const auto& instances = setIt->second;
    if (instanceIndex >= instances.size()) {
        throw std::runtime_error("Instance index " + std::to_string(instanceIndex) + " is out of bounds for set " + std::to_string(setIndex) + " (size: " + std::to_string(instances.size()) + ").");
    }
    return instances[instanceIndex];
}

void DescriptorManager::bindBufferToSet(uint32_t layoutIdx,
                                        uint32_t setInstance,
                                        uint32_t binding,
                                        vk::Buffer buffer,
                                        vk::DeviceSize size) {
    vk::DescriptorBufferInfo bufferInfo{buffer, 0, size};

    vk::WriteDescriptorSet write{};
    write.setDstSet(m_sets[layoutIdx][setInstance]);
    write.setDstBinding(binding);
    write.setDstArrayElement(0);
    write.setDescriptorCount(1);
    write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    write.setPImageInfo(nullptr);
    write.setPBufferInfo(&bufferInfo);

    m_context->getDevice().updateDescriptorSets(write, {});
}

void DescriptorManager::bindImageToSet(uint32_t layoutIdx,
                                       uint32_t setInstance,
                                       uint32_t binding,
                                       vk::ImageView imageView,
                                       vk::Sampler sampler) {
    vk::DescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    vk::WriteDescriptorSet write{};
    write.setDstSet(m_sets[layoutIdx][setInstance]);
    write.setDstBinding(binding);
    write.setDstArrayElement(0);
    write.setDescriptorCount(1);
    write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    write.setPImageInfo(&imageInfo);
    write.setPBufferInfo(nullptr);

    m_context->getDevice().updateDescriptorSets(write, {});
}

vk::DescriptorSet DescriptorManager::getDescriptorSet(uint32_t setIndex, uint32_t index) const {
    auto setIt = m_sets.find(setIndex);
    if (setIt == m_sets.end()) {
        throw std::runtime_error("Sets for set " + std::to_string(setIndex) + " have not been allocated. Call allocateAllSets() first.");
    }
    const auto& instances = setIt->second;
    if (index >= instances.size()) {
        throw std::runtime_error("Instance index " + std::to_string(index) + " is out of bounds for set " + std::to_string(setIndex) + " (size: " + std::to_string(instances.size()) + ").");
    }
    return instances[index];
}

std::vector<vk::DescriptorSetLayout> DescriptorManager::getAllDescriptorSetLayouts() const {
    if (m_layouts.empty()) {
        return {};
    }
    // 找到最大的 set index
    uint32_t maxSetIndex = 0;
    for (const auto& kv : m_layouts) {
        maxSetIndex = std::max(maxSetIndex, kv.first);
    }
    // 创建一个大小为 maxSetIndex+1 的 vector
    std::vector<vk::DescriptorSetLayout> layouts(maxSetIndex + 1, nullptr);
    // 将每个 layout 放到其对应的索引位置
    for (const auto& kv : m_layouts) {
        layouts[kv.first] = kv.second;
    }
    // 可选：检查是否有空洞（Vulkan 不允许 pipeline layout 有空洞）
    for (size_t i = 0; i < layouts.size(); ++i) {
        if (layouts[i] == nullptr) {
            throw std::runtime_error("Descriptor set layout for set " + std::to_string(i) + " is missing. Pipeline layouts must be dense from set 0.");
        }
    }
    return layouts;
}

// 模板显式实例化
// 注意：使用值类型而非指针类型，匹配 Renderer.cpp 中的调用
template void DescriptorManager::createLayout<CameraUBO>(uint32_t, uint32_t);
template void DescriptorManager::createLayout<TransformUBO, LightUBO, MaterialUBO, TextureSampler, TextureSampler, TextureSampler, TextureSampler>(uint32_t, uint32_t);

