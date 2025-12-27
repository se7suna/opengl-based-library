
cmake_minimum_required(VERSION 3.10)
project(OpenGLProject)


# Force UTF-8 source encoding on MSVC
if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 17)

# ===== 配置 GLAD =====
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ===== 配置 ImGui =====
# 注意：需要先下载ImGui到external/imgui目录
# 详细说明请参考 IMGUI_SETUP.md
# 下载地址：https://github.com/ocornut/imgui/releases
# 或使用: git clone https://github.com/ocornut/imgui.git external/imgui
file(GLOB IMGUI_SOURCES 
    "${CMAKE_SOURCE_DIR}/external/imgui/*.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp"
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    "${CMAKE_SOURCE_DIR}/external/imgui"
    "${CMAKE_SOURCE_DIR}/external/imgui/backends"
    "${CMAKE_SOURCE_DIR}/external/glfw/include"  # ImGui后端需要GLFW头文件
)

# ===== 生成可执行文件 (exe) =====
# 把 Shader.cpp 也加进去，不然链接的时候会报错
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/Shader.cpp
    src/Mesh.cpp
    src/Model.cpp
    src/Camera.cpp
    src/Texture.cpp
    src/ProceduralPlant.cpp
    src/Scene.cpp
    src/ShadowManager.cpp
)

# ===== 头文件包含路径 =====
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/glfw/include
)


# Pass asset root directory to the executable (so it can find shaders/models/materials regardless of working directory)
target_compile_definitions(${PROJECT_NAME} PRIVATE ASSET_DIR="${CMAKE_SOURCE_DIR}")
# ===== 设置 GLFW 库文件 (Windows) =====
set(GLFW_LIB
    ${CMAKE_SOURCE_DIR}/external/glfw/lib-vc2022/glfw3.lib
)

# ===== 链接库 =====
# 链接 glad, glfw, imgui, 还有 Windows 自带的 opengl32
target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    imgui
    ${GLFW_LIB}
    opengl32
)
# ====== Copy runtime assets next to the exe so relative paths work ======
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/models"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/materials"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/materials"
)
