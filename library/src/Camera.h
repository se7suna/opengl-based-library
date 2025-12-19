#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 第一人称相机类
class Camera {
public:
    // 相机属性
    glm::vec3 Position;      // 相机位置
    glm::vec3 Front;         // 相机前方向（朝向）
    glm::vec3 Up;            // 相机上方向
    glm::vec3 Right;          // 相机右方向
    glm::vec3 WorldUp;       // 世界坐标系的上方向

    // 欧拉角
    float Yaw;               // 偏航角（左右旋转）
    float Pitch;             // 俯仰角（上下旋转）

    // 相机选项
    float MovementSpeed;     // 移动速度
    float MouseSensitivity;  // 鼠标灵敏度
    float Zoom;              // 视野（FOV）

    // 构造函数
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f);

    // 获取视图矩阵
    glm::mat4 GetViewMatrix();

    // 处理键盘输入（WASD移动）
    void ProcessKeyboard(int direction, float deltaTime);

    // 处理鼠标移动输入
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // 处理鼠标滚轮输入（缩放）
    void ProcessMouseScroll(float yoffset);

private:
    // 根据欧拉角更新相机向量
    void updateCameraVectors();
};

#endif

