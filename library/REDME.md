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

#### Task 4: PBR 材质与着色器 ✅
- **PBR 顶点/片元着色器** (`shaders/pbr.vert`, `shaders/pbr.frag`)
  - 实现 Cook-Torrance PBR BRDF（GGX NDF + Smith 几何项 + Schlick 菲涅尔）
  - 使用 Metallic-Roughness 工作流
  - 支持多点光源（当前使用 4 盏点光源，2x2 排布）
  - 通过 `sampler2D` 采样 `albedo/metallic/roughness/ao` 贴图
  - 支持 Triplanar Mapping（用于墙壁等大平面物体）
  - 支持传统 UV 映射（用于地板等有正确 UV 的模型）
- **PBR 纹理材质加载** (`src/Texture.h`, `src/Texture.cpp`)
  - 基于 `stb_image` 的 2D 纹理加载封装 `LoadTexture2D(path, srgb)`
  - 定义 `PBRTextureMaterial` 结构体，统一管理一套 PBR 贴图（albedo/normal/metallic/roughness/ao）
  - 支持多种 PBR 材质加载：
    - 橡木材质（WoodVeneerOak_7760）- 用于书架、桌子
    - 木地板材质（WoodFloorAsh_4186）- 用于地板
    - 金属材质（MetalGalvanizedZinc_7184）- 用于饮水机
    - 喷漆金属材质（MetalPaintedMatte_7037）- 用于顶灯
    - 皮革材质（FabricLeatherCowhide_001）- 用于椅子
    - 大理石材质（TilesTravertine_001）- 用于墙面、天花板

#### Task 5: 动态光照与阴影 ✅
- **点光源系统**
  - 实现 4 个点光源（2x2 排布在天花板上）
  - 每个光源独立控制位置、颜色和强度
  - 光源强度：150.0（高亮度配置）
  - 支持距离衰减（平方反比）
- **实时阴影映射（Shadow Mapping）**
  - 为每个光源创建独立的阴影贴图（2048x2048 分辨率）
  - 使用正交投影从光源视角渲染深度贴图
  - 阴影贴图 FBO 和深度纹理管理
- **阴影采样与过滤**
  - 实现 PCF（Percentage-Closer Filtering）软阴影
  - 3x3 采样，提供平滑的阴影边缘
  - 动态阴影偏移（bias）计算，避免阴影失真
  - 阴影范围检查，处理超出阴影贴图范围的情况
- **阴影着色器** (`shaders/shadow.vert`, `shaders/shadow.frag`)
  - 简化的深度渲染着色器
  - 从光源视角渲染场景到深度贴图
- **PBR 着色器阴影集成**
  - 在 PBR 片元着色器中集成阴影采样
  - 每个光源独立计算阴影因子
  - 阴影与 PBR 光照正确混合

#### Task 6: 场景构建 ✅
- **图书馆场景布局**
  - 25x40 单位的大型场景
  - 6 排桌子，每排 12 张桌子短边相连
  - 每排桌子两侧各 8 把椅子
  - 每排桌子一端放置 2 个背靠背的书架
  - 4 个顶灯（2x2 排布）照亮整个场景
  - 饮水机放置在角落
- **模型资源**
  - 书架模型（bookshelf.obj）
  - 桌子模型（library_table.obj）
  - 椅子模型（stool.obj）
  - 饮水机模型（water_dispenser.obj）
  - 基础几何体（cube.obj, sphere.obj）
- **场景元素**
  - 地板、四面墙、天花板
  - 所有物体使用 PBR 材质渲染
  - 合理的物体位置和缩放

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
│   ├── basic.frag          # 片段着色器（Blinn-Phong 光照）
│   ├── pbr.vert            # PBR 顶点着色器
│   ├── pbr.frag            # PBR 片元着色器（Cook-Torrance BRDF）
│   ├── shadow.vert          # 阴影映射顶点着色器
│   └── shadow.frag          # 阴影映射片元着色器
│
├── models/                 # 3D 模型文件
│   ├── cube.obj            # 立方体（用于地板、墙壁、天花板）
│   ├── sphere.obj          # 球体（用于顶灯）
│   ├── bookshelf.obj       # 书架模型
│   ├── library_table.obj   # 图书馆桌子模型
│   ├── stool.obj           # 椅子模型
│   └── water_dispenser.obj # 饮水机模型
├── materials/              # PBR 材质贴图
│   ├── Poliigon_WoodVeneerOak_7760/      # 橡木材质
│   ├── Poliigon_WoodFloorAsh_4186_Preview1/  # 木地板材质
│   ├── Poliigon_MetalGalvanizedZinc_7184/    # 金属材质
│   ├── Poliigon_MetalPaintedMatte_7037_Preview1/  # 喷漆金属材质
│   ├── FabricLeatherCowhide001/           # 皮革材质
│   └── TilesTravertine001/                 # 大理石材质
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
  ├─ 加载 PBR 材质贴图
  ├─ 创建着色器 (PBR Shader, Shadow Shader)
  ├─ 创建阴影贴图 FBO（4 个光源）
  └─ 渲染循环
      ├─ 计算帧时间 (deltaTime)
      ├─ 处理键盘输入 (WASD)
      ├─ 处理鼠标输入 (视角旋转)
      ├─ 第一步：渲染阴影贴图（从每个光源视角）
      │   ├─ 计算光源位置和光源空间矩阵
      │   ├─ 为每个光源渲染场景到深度贴图
      │   └─ 生成 4 张阴影贴图
      ├─ 第二步：渲染主场景（应用阴影）
      │   ├─ 更新视图和投影矩阵
      │   ├─ 绑定阴影贴图到纹理单元
      │   ├─ 设置光源参数（位置、颜色、强度）
      │   ├─ 渲染场景物体（地板、墙壁、书架、桌子、椅子等）
      │   └─ PBR 着色器应用阴影和光照
      └─ 交换缓冲区
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

#### 2. 动态光照与阴影 ⭐⭐⭐ ✅
- [x] 实现点光源系统（4 个环境内灯光，2x2 排布）
- [x] 实现实时阴影映射（Shadow Mapping）
  - [x] 深度贴图生成（每个光源独立阴影贴图）
  - [x] 阴影采样与过滤（PCF 软阴影）
  - [x] 阴影偏移和范围检查
- [ ] 实现方向光（模拟自然光，可选）
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

#### 6. 场景构建 ⭐⭐ ✅
- [x] 设计图书馆布局（大型图书馆场景）
- [x] 制作/获取模型资源：
  - [x] 书架模型
  - [x] 桌椅模型
  - [x] 饮水机模型
  - [x] 基础几何体（立方体、球体）
- [x] 准备 PBR 材质贴图（6 种不同材质）
- [x] 场景渲染（在 main.cpp 中实现完整场景）
- [ ] 场景管理类（Scene 或 Level，可选优化）

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
  - 使用 Cook-Torrance BRDF 模型
  - GGX 法线分布函数（NDF）
  - Smith 几何项（Geometry Term）
  - Schlick 菲涅尔近似
- **Shadow Mapping**: 实时阴影生成技术
  - 从光源视角渲染深度贴图
  - 在主渲染通道中比较深度值判断是否在阴影中
  - PCF（Percentage-Closer Filtering）提供软阴影效果
- **IBL (Image-Based Lighting)**: 使用环境贴图模拟全局光照（待实现）
- **SSAO (Screen-Space Ambient Occlusion)**: 屏幕空间环境光遮蔽（待实现）

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
- **Task 4**: PBR 材质系统实现（Cook-Torrance BRDF、多种材质支持、Triplanar Mapping）
- **Task 5**: 动态光照与阴影系统（4 个点光源、实时阴影映射、PCF 软阴影）
- **Task 6**: 图书馆场景构建（完整场景布局、模型资源、PBR 材质应用）

---

## 联系方式

如有问题，请联系团队成员或查阅项目文档。
