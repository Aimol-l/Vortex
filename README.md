# Vortex - Vulkan PBR Renderer

一个基于 Vulkan 的 3D 渲染引擎，实现了基于物理的渲染（PBR）管线。学习使用。

## 特性

- **现代 Vulkan API**: 使用 Vulkan-Hpp 绑定，遵循现代 C++17 标准
- **基于物理的渲染 (PBR)**: 完整的 Cook-Torrance BRDF 实现
- **PBR 纹理支持**:
  - Albedo (漫反射) 纹理
  - Normal (法线) 贴图
  - Metallic (金属度) 纹理
  - Roughness (粗糙度) 纹理
- **多物体场景**: 支持任意数量的动态物体
- **自动旋转**: 场景中的物体可自动旋转展示
- **相机系统**: WASD 移动 + 鼠标控制的 FPS 风格相机
- **窗口自适应**: 支持窗口大小调整和 Swapchain 重建
- **高效的资源管理**:
  - VMA (Vulkan Memory Allocator) 集成
  - 描述符池管理
  - 多帧 in-flight 同步

## 系统要求

### 依赖库
- GLFW 3.4+
- GLM (数学库)
- Vulkan Memory Allocator (VMA)
- SPIRV-Tools (着色器编译)
- stb_image (纹理加载)

## 编译说明

### 2. 编译项目

```bash
# 克隆仓库
git clone 

cd Vortex

cmake -B build && cmake --build build 

./bin/vortex

```

## 控制说明

### 相机控制
- **W**: 前进
- **S**: 后退
- **A**: 向左
- **D**: 向右
- **鼠标左键**: 按住捕获鼠标，移动鼠标旋转视角
- **ESC**: 退出程序

## 项目结构

```
Vortex/
├── assets/              # 美术资源（模型、纹理）
│   ├── Cube.obj
│   ├── Cube_Diffuse.jpg
│   ├── Cube_Normal.jpg
│   ├── Cube_Glossyness.jpg
│   └── Cube_Roughness.jpg
├── include/             # 头文件
│   ├── Core/           # 核心渲染系统
│   │   ├── Application.h
│   │   ├── Context.h
│   │   ├── Renderer.h
│   │   ├── Command.h
│   │   ├── Swapchain.h
│   │   ├── Descriptor.h
│   │   ├── Pipeline.h
│   │   └── Window.h
│   ├── Scene/          # 场景管理
│   │   ├── Scene.h
│   │   ├── Camera.h
│   │   └── Renderable.h
│   └── Assets/         # 资源管理
│       ├── Mesh.h
│       ├── Material.h
│       └── Texture.h
├── src/                # 源文件实现
├── shaders/            # GLSL 着色器
│   ├── pbr.vert        # PBR 顶点着色器
│   └── pbr.frag        # PBR 片段着色器
├── test/               # 测试程序
│   └── vortex.cpp      # 主入口
└── build/              # 构建输出
```

## 渲染管线

### 1. 初始化阶段
```
Window → Context → Swapchain → RenderPass → Pipeline → Descriptors → Framebuffers
```

### 2. 每帧渲染循环
```
1. 等待 Fence (CPU-GPU 同步)
2. 获取 Swapchain 图像
3. 录制命令缓冲:
   - 开始渲染通道
   - 绑定管线和描述符
   - 绘制物体
   - 结束渲染通道
4. 提交命令到队列
5. 呈现图像
```

### 3. 同步机制
- **Frames in Flight**: 2 帧并行渲染
- **Semaphores**: GPU 内部同步（图像获取、渲染完成）
- **Fences**: CPU-GPU 同步（防止 CPU 提交过快）

## PBR 渲染

项目实现了完整的 Cook-Torrance 微表面 BRDF 模型：

- **法线分布函数**: Trowbridge-Reitz GGX
- **几何函数**: Smith (Schlick-GGX)
- **菲涅尔方程**: Schlick 近似

### 材质参数
- **Albedo**: 基础反射颜色
- **Metallic**: 0.0 (非金属) ~ 1.0 (纯金属)
- **Roughness**: 0.0 (光滑) ~ 1.0 (粗糙)
- **AO**: 环境光遮蔽

## 性能优化

- **批处理**: 多物体共享材质和网格
- **描述符复用**: 每帧复用描述符集
- **内存管理**: VMA 自动管理显存分配
- **帧率限制**: 默认 60 FPS，避免 GPU 过载

## 已知问题与 TODO

### 已实现 ✅
- [x] Vulkan 基础架构
- [x] PBR 材质系统
- [x] 纹理加载与采样
- [x] 多物体渲染
- [x] 相机控制系统
- [x] 窗口自适应
- [x] 自动旋转动画

### 待实现 📋
- [ ] 阴影贴图 (Shadow Mapping)
- [ ] 多光源支持
- [ ] IBL (基于图像的照明)
- [ ] 后处理效果 (Bloom, Tone Mapping)
- [ ] 延迟渲染管线
- [ ] 脊柱/骨骼动画
- [ ] 粒子系统
- [ ] 性能分析工具

## 故障排除

### 验证层错误
如果遇到 Vulkan 验证层警告，确保：
1. 安装了 Vulkan Validation Layers
2. 着色器已正确编译为 SPIR-V
3. 描述符正确绑定

### 编译错误
- 确保 C++17 支持
- 检查 Vulkan SDK 版本
- 验证所有依赖库已安装

### 运行时崩溃
- 检查 GPU 驱动是否支持 Vulkan
- 确认 `assets/` 文件夹存在且包含所需资源
- 查看终端输出的错误信息

## 学习资源

本项目参考了以下资源：
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Learn OpenGL - PBR Theory](https://learnopengl.com/PBR/Theory)
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/1.3/html/)

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License

## 作者

Created with ❤️ using Vulkan and C++17

---

**注意**: 这是一个学习项目，主要用于理解 Vulkan API 和现代渲染技术。生产环境使用请考虑更成熟的引擎（如 Unreal、Unity 或 Godot）。
