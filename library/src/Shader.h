#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// 完整的 Shader 类，声明所有成员函数（与实现一致）
class Shader {
public:
    unsigned int ID;

    // 构造函数（声明）
    Shader(const char* vertexPath, const char* fragmentPath);

    // 成员函数声明（必须与 .cpp 实现一致，包括 const 修饰）
    void use() const;
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif