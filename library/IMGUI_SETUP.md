# ImGui 集成说明

本项目使用了 ImGui 来实现时间控制滑动条。在使用前，需要先下载并集成 ImGui。

## 下载 ImGui

### 方法 1：使用 Git（推荐）

在项目根目录（`opengl-based-library/library`）执行：

```bash
cd external
git clone https://github.com/ocornut/imgui.git
```

### 方法 2：手动下载

1. 访问 https://github.com/ocornut/imgui/releases
2. 下载最新版本的源码（例如 `imgui-1.xx.x.zip`）
3. 解压到 `library/external/imgui` 目录

## 所需文件

确保以下文件存在：

```
library/external/imgui/
  ├── imgui.h
  ├── imgui.cpp
  ├── imgui_demo.cpp
  ├── imgui_draw.cpp
  ├── imgui_internal.h
  ├── imgui_tables.cpp
  ├── imgui_widgets.cpp
  └── backends/
      ├── imgui_impl_glfw.h
      ├── imgui_impl_glfw.cpp
      ├── imgui_impl_opengl3.h
      └── imgui_impl_opengl3.cpp
```

## 编译

完成下载后，使用 CMake 重新配置和编译项目即可。ImGui 会自动被编译并链接到项目中。

## 功能说明

- 右上角的时间滑动条可以调节虚拟时间（0-24小时）
- 默认时间为中午12点
- 时间改变会影响：
  - 自然光（太阳光）的方向，24小时内绕场景一圈
  - 顶灯的开关状态（6-18点关闭，18-6点打开）

