# 探索图书馆：基于 PBR 的实时渲染

## 项目概述

本项目是一个基于 OpenGL 的实时渲染项目，目标是构建一个具备实时交互性的图书馆场景，采用 PBR（基于物理的渲染）等渲染技术，在保证视觉效果的同时兼顾性能。

**团队成员**: 方天辰、刘家合、栗田新那、马逸轩

---

## 当前项目状态

### ✅ 已完成功能

#### Task 1: 基础渲染管线
- **基础环境搭建**
  - 集成 GLFW（窗口管理）和 GLAD（OpenGL 加载器）
  - 所有第三方库文件位于 `external/` 目录
  - 支持 OpenGL 3.3 Core Profile

- **Shader 类封装** (`src/Shader.h`, `src/Shader.cpp`)
  - 自动读取、编译、链接着色器文件
  - 提供便捷的 uniform 设置接口：
    - `setBool()`, `setInt()`, `setFloat()`
    - `setVec3()` (两种重载)
    - `setMat4()`
  - 完整的错误检查机制

- **深度测试**
  - 已启用 `GL_DEPTH_TEST`
  - 正确处理物体遮挡关系

#### Task 2: 模型加载系统
- **Mesh 类** (`src/Mesh.h`, `src/Mesh.cpp`)
  - 封装单个网格的顶点数据（位置、法线、纹理坐标）
  - 管理 VAO、VBO、EBO 缓冲区
  - 提供 `Draw()` 方法进行渲染

- **Model 类** (`src/Model.h`, `src/Model.cpp`)
  - 支持 OBJ 文件加载
  - 解析顶点位置、法线、纹理坐标和面索引（支持 `v/vt/vn` 等常见格式）
  - 支持多网格模型（一个 Model 包含多个 Mesh）
  - 自动计算法线（当 OBJ 文件缺少法线时）
  - 当 OBJ 不含纹理坐标时，会根据顶点位置自动生成简单 UV（保证 PBR 贴图至少能看到变化）

- **基础光照**
  - 实现 Blinn-Phong 光照模型
  - 支持两个方向光（dirLights[0] 和 dirLights[1]）
  - 材质系统（diffuse、specular、shininess）

#### Task 3: 第一人称相机控制
- **Camera 类** (`src/Camera.h`, `src/Camera.cpp`)
  - 实现第一人称相机系统
  - 位置、方向、上向量管理
  - 欧拉角控制（偏航角 Yaw、俯仰角 Pitch）
  - 键盘移动控制（WASD）
  - 鼠标视角控制（平滑旋转）
  - 鼠标滚轮缩放（FOV 调整）
  - 帧时间同步，确保移动速度一致
  - 支持按下 `Ctrl` 键在“捕获鼠标（视角控制）/ 释放鼠标（操作 UI）”之间切换

#### Task 4: PBR 材质与着色器（进行中）
- **PBR 顶点/片元着色器** (`shaders/pbr.vert`, `shaders/pbr.frag`)
  - 实现 Cook-Torrance PBR BRDF（GGX NDF + Smith 几何项 + Schlick 菲涅尔）
  - 使用 Metallic-Roughness 工作流
  - 支持多点光源（当前使用 2 盏点光）
  - 通过 `sampler2D` 采样 `albedo/metallic/roughness/ao` 贴图（法线贴图预留接口，后续可接入 TBN）
- **PBR 纹理材质加载** (`src/Texture.h`, `src/Texture.cpp`)
  - 基于 `stb_image` 的 2D 纹理加载封装 `LoadTexture2D(path, srgb)`
  - 定义 `PBRTextureMaterial` 结构体，统一管理一套 PBR 贴图（albedo/normal/metallic/roughness/ao）
  - 针对 `Poliigon_WoodVeneerOak_7760` 橡木纹材质实现专门的加载函数：
    - 使用 `BaseColor/Normal/Metallic/Roughness/AmbientOcclusion` 五张贴图
  - 在 `main.cpp` 中将该材质应用到 `model1` 上（左侧模型，当前为 `geodesic_classI_2.obj`）
  - 保留原有 Blinn-Phong 管线用于对比（右侧 `sphere.obj`）

---

## 项目结构

```
library/
├── CMakeLists.txt          # CMake 构建配置
├── REDME.md                # 项目文档（本文件）
│
├── src/                    # 源代码目录
│   ├── main.cpp            # 主程序入口
│   ├── Shader.h/cpp        # 着色器封装类
│   ├── Mesh.h/cpp          # 网格类
│   ├── Model.h/cpp         # 模型加载类
│   └── Camera.h/cpp        # 第一人称相机类
│
├── shaders/                # 着色器文件
│   ├── basic.vert          # 顶点着色器（MVP 变换、法线变换）
│   └── basic.frag          # 片段着色器（Blinn-Phong 光照）
│
├── models/                 # 3D 模型文件
│   ├── cube.obj
│   ├── sphere.obj
│   ├── geodesic_classI_2.obj
│   ├── roadBike.obj
│   └── simple_triangle.obj
│
└── external/               # 第三方库
    ├── glad/               # GLAD (OpenGL 加载器)
    │   ├── include/
    │   └── src/
    ├── glfw/               # GLFW (窗口管理)
    │   ├── include/
    │   └── lib-vc2022/     # Visual Studio 2022 库文件
    └── glm/                # GLM (数学库)
```

---

## 代码架构说明

### 核心类设计

1. **Shader 类**
   - 职责：管理 OpenGL 着色器程序的完整生命周期
   - 功能：文件读取、编译、链接、uniform 设置
   - 使用示例：
     ```cpp
     Shader shader("path/to/vertex.vert", "path/to/fragment.frag");
     shader.use();
     shader.setMat4("model", modelMatrix);
     shader.setVec3("material.diffuse", glm::vec3(0.8f));
     ```

2. **Mesh 类**
   - 职责：封装单个网格的渲染数据
   - 数据结构：`Vertex`（位置、法线、纹理坐标）
   - 渲染：通过 VAO/VBO/EBO 高效渲染

3. **Model 类**
   - 职责：管理完整模型（可能包含多个 Mesh）
   - 功能：OBJ 文件解析、网格构建
   - 渲染：遍历所有 Mesh 并依次渲染

4. **Camera 类**
   - 职责：管理第一人称相机
   - 功能：位置、方向管理，视图矩阵计算
   - 控制：WASD 移动、鼠标视角旋转、滚轮缩放
   - 使用示例：
     ```cpp
     Camera camera(glm::vec3(0.0f, 0.5f, 5.0f));
     camera.ProcessKeyboard(0, deltaTime);  // W键前进
     camera.ProcessMouseMovement(xoffset, yoffset);
     glm::mat4 view = camera.GetViewMatrix();
     ```

### 当前渲染流程

```
main.cpp
  ├─ 初始化 GLFW/GLAD
  ├─ 创建相机 (Camera)
  ├─ 设置鼠标/键盘回调
  ├─ 加载模型 (Model::loadOBJ)
  ├─ 创建着色器 (Shader)
  ├─ 设置光照参数
  └─ 渲染循环
      ├─ 计算帧时间 (deltaTime)
      ├─ 处理键盘输入 (WASD)
      ├─ 处理鼠标输入 (视角旋转)
      ├─ 更新视图矩阵 (Camera::GetViewMatrix)
      ├─ 更新投影矩阵 (根据窗口大小和FOV)
      ├─ 设置 uniform
      └─ Model::Draw() → Mesh::Draw()
```

---

## 开发环境配置

### 必需组件

1. **Visual Studio 2022** (或 Visual Studio Community 2022)
   - 包含 MSVC 编译器
   - 下载地址：https://visualstudio.microsoft.com/

2. **CMake** (3.10 或更高版本)
   - 下载地址：https://cmake.org/download/
   - 安装时选择"添加到系统 PATH"

3. **VS Code** (可选，推荐)
   - 安装扩展：**CMake Tools**
   - 用于更好的 CMake 集成体验

### 依赖库说明

项目已包含以下依赖库（位于 `external/` 目录）：

- **GLAD**: OpenGL 函数加载器（已包含）
- **GLFW**: 窗口和输入管理（已包含，Windows x64）
- **GLM**: OpenGL 数学库（已包含）

**注意**: 如果 `external/` 目录不完整，请参考下方的"依赖安装指南"。

---

## 编译与运行

### 使用 VS Code + CMake Tools

1. **打开项目**
   - 在 VS Code 中打开项目根目录

2. **配置 CMake**
   - 按 `Ctrl+Shift+P`，输入 "CMake: Configure"
   - 选择编译器：`Visual Studio Community 2022 Release - amd64`

3. **编译**
   - 按 `F7` 或点击状态栏的 "Build" 按钮
   - 或使用命令：`CMake: Build`

4. **运行**
   - 编译后的可执行文件位于 `build/Debug/OpenGLProject.exe`
   - **重要**: 确保 `shaders/` 、 `models/` 和 `materials/`  文件夹与 `.exe` 文件在同一目录

### 使用命令行

```bash
# 在项目根目录执行
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# 运行（需要先复制 shaders 和 models 到 build/Release/）
```

### 常见问题

- **窗口一闪而退**: 通常是找不到 `shaders/` 或 `models/` 或 `materials/` 文件夹
  - 解决：将这三个文件夹复制到 `.exe` 文件同级目录
- **编译乱码**: 检查源文件编码是否为 **UTF-8 with BOM**
- **链接错误**: 确认 `external/glfw/lib-vc2022/` 目录存在且包含 `.lib` 文件

---

## 未来任务规划

根据项目提案，以下是待实现的功能模块：

### 🎯 核心目标功能

#### 1. PBR 材质系统 ⭐⭐⭐
- [x] 实现基于物理的渲染（Cook-Torrance BRDF）
- [x] 支持 PBR 材质贴图：
  - [x] 反照率贴图 (Albedo/Diffuse)
  - [x] 法线贴图 (Normal Map)
  - [x] 金属度贴图 (Metallic)
  - [x] 粗糙度贴图 (Roughness)
  - [x] 环境光遮蔽贴图 (AO)
- [x] 替换当前 Blinn-Phong 光照模型
- [x] 创建 PBR 着色器（`pbr.vert`, `pbr.frag`）

**参考资源**:
- [LearnOpenGL - PBR Theory](https://learnopengl.com/PBR/Theory)
- Real Shading in Unreal Engine 4 (Brian Karis)

#### 2. 动态光照与阴影 ⭐⭐⭐
- [ ] 实现点光源系统（多个环境内灯光）
- [ ] 实现方向光（模拟自然光）
- [ ] 实现实时阴影映射（Shadow Mapping）
  - [ ] 深度贴图生成
  - [ ] 阴影采样与过滤（PCF/PCSS）
- [ ] 优化阴影性能（级联阴影贴图可选）

#### 3. 基于图像的光照 (IBL) ⭐⭐
- [ ] 环境贴图加载（HDR 或 Cubemap）
- [ ] 预计算辐照度贴图（Irradiance Map）
- [ ] 预计算镜面反射贴图（Prefilter Map）
- [ ] BRDF 查找表（LUT）
- [ ] 在 PBR 着色器中集成 IBL

**参考资源**:
- [LearnOpenGL - IBL](https://learnopengl.com/PBR/IBL)

#### 4. 后处理效果 ⭐⭐
- [ ] 屏幕空间环境光遮蔽 (SSAO)
  - [ ] 生成法线和深度缓冲
  - [ ] SSAO 计算着色器
  - [ ] 模糊处理
- [ ] 色调映射（HDR → LDR，可选）
- [ ] 后处理管线框架（便于扩展）

#### 5. 第一人称相机控制 ⭐ ✅
- [x] 实现 `Camera` 类
  - [x] 位置、方向、上向量管理
  - [x] 鼠标视角控制（俯仰角、偏航角）
  - [x] 键盘移动（WASD）
- [x] 集成到主循环
- [x] 平滑移动和视角旋转（基于帧时间）

#### 6. 场景构建 ⭐⭐
- [ ] 设计图书馆布局（参考嘉定图书馆）
- [ ] 制作/获取模型资源：
  - [ ] 书架模型
  - [ ] 桌椅模型
  - [ ] 窗户模型
  - [ ] 其他装饰物
- [ ] 准备 PBR 材质贴图
- [ ] 场景管理类（Scene 或 Level）

### 📅 实施计划（参考）

- **第 9 周**: 场景设计、模型和纹理准备
- **第 10 周**: PBR 着色器实现、基础材质系统
- **第 11 周**: 动态光照、阴影系统
- **第 12 周**: IBL 环境反射、SSAO 后处理
- **第 13 周**: 第一人称相机、交互优化
- **第 14 周+**: 性能优化、Bug 修复、最终测试

### 🚫 明确排除的功能

- 复杂物理模拟（碰撞反馈、可交互物体）
- 角色动画或 NPC
- 游戏机制（任务、解谜等）
- 跨平台支持（仅 Windows）
- 高细节动态物体（飘动窗帘、粒子系统）
- 高级后处理（运动模糊、路径追踪）

---

## 技术参考

### 主要参考资料

1. **Real Shading in Unreal Engine 4** - Brian Karis (Epic Games)
2. **Real-Time Rendering, Fourth Edition** - Tomas Akenine-Möller, Eric Haines, Naty Hoffman
3. **OpenGL 官方文档** - Khronos Group
4. **LearnOpenGL.com** - 优秀的 OpenGL 教程网站

### 关键概念

- **PBR (Physically Based Rendering)**: 基于微平面理论、能量守恒和菲涅尔反射
- **IBL (Image-Based Lighting)**: 使用环境贴图模拟全局光照
- **Shadow Mapping**: 实时阴影生成技术
- **SSAO (Screen-Space Ambient Occlusion)**: 屏幕空间环境光遮蔽

---

## 代码规范与注意事项

1. **文件编码**: 所有源文件使用 **UTF-8 with BOM**（避免中文乱码）
2. **路径处理**: 避免硬编码绝对路径，使用相对路径或资源管理器
3. **错误处理**: 所有文件操作和 OpenGL 调用都应包含错误检查
4. **性能目标**: 保持至少 **30 FPS** 的运行帧率

---

## 更新日志

- **Task 1**: 基础渲染管线、Shader 类封装
- **Task 2**: Mesh/Model 类、OBJ 加载、Blinn-Phong 光照、自动法线计算
- **Task 3**: 第一人称相机控制（WASD 移动、鼠标视角控制）

---

## 联系方式

如有问题，请联系团队成员或查阅项目文档。
