#pragma once
#include <string>
#include <map>
#include "raylib.h"

struct ShaderProperty {
    std::string type;
    std::string defaultValue;
};

struct ShaderDef {
    std::string name;
    std::string type; // "Lit", "Unlit", "PostProcess"
    std::map<std::string, ShaderProperty> properties;
    std::string fragmentLogic;
    std::string vertexLogic;
};

class ShaderBuilder {
public:
    static Shader GenerateShader(const ShaderDef& def);
private:
    static std::string BuildVertexShader(const ShaderDef& def);
    static std::string BuildFragmentShader(const ShaderDef& def);
};
