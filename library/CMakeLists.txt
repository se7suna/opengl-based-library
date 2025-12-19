
cmake_minimum_required(VERSION 3.10)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)

# ===== 配置 GLAD =====
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ===== 生成可执行文件 (exe) =====
# 把 Shader.cpp 也加进去，不然链接的时候会报错
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/Shader.cpp
    src/Mesh.cpp
    src/Model.cpp
    src/Camera.cpp
    src/Texture.cpp
)

# ===== 头文件包含路径 =====
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/glfw/include
)

# ===== 设置 GLFW 库文件 (Windows) =====
set(GLFW_LIB
    ${CMAKE_SOURCE_DIR}/external/glfw/lib-vc2022/glfw3.lib
)

# ===== 链接库 =====
# 链接 glad, glfw, 还有 Windows 自带的 opengl32
target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    ${GLFW_LIB}
    opengl32
)