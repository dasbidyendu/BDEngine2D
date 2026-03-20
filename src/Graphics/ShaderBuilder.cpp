#include "ShaderBuilder.h"
#include "Utils/Logger.h"

Shader ShaderBuilder::GenerateShader(const ShaderDef& def) {
    std::string vsCode = BuildVertexShader(def);
    std::string fsCode = BuildFragmentShader(def);
    
    // Load shader from strings
    Shader shader = LoadShaderFromMemory(vsCode.c_str(), fsCode.c_str());
    if (shader.id == 0) {
        Logger::AddLog(LOG_LEVEL_ERROR, "Failed to compile shader: %s", def.name.c_str());
    } else {
        Logger::AddLog(LOG_LEVEL_SUCCESS, "Successfully compiled shader: %s", def.name.c_str());
    }
    return shader;
}

std::string ShaderBuilder::BuildVertexShader(const ShaderDef& def) {
    if (!def.vertexLogic.empty()) {
        return def.vertexLogic;
    }
    return R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec2 fragWorldPos;
uniform mat4 mvp;
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragWorldPos = vertexPosition.xy;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";
}

std::string ShaderBuilder::BuildFragmentShader(const ShaderDef& def) {
    std::string fs = "#version 330\n";
    fs += "in vec2 fragTexCoord;\n";
    fs += "in vec4 fragColor;\n";
    fs += "in vec2 fragWorldPos;\n";
    fs += "out vec4 finalColor;\n";
    
    fs += "uniform sampler2D texture0;\n";
    
    for (const auto& kv : def.properties) {
        if (kv.second.type == "texture2D") {
            fs += "uniform sampler2D " + kv.first + ";\n";
        } else if (kv.second.type == "color" || kv.second.type == "vec4") {
            fs += "uniform vec4 " + kv.first + ";\n";
        } else if (kv.second.type == "float") {
            fs += "uniform float " + kv.first + ";\n";
        } else if (kv.second.type == "vec2") {
            fs += "uniform vec2 " + kv.first + ";\n";
        } else if (kv.second.type == "vec3") {
            fs += "uniform vec3 " + kv.first + ";\n";
        }
    }
    
    if (def.type == "Lit") {
        fs += "struct Light {\n";
        fs += "    int type;\n";
        fs += "    vec4 color;\n";
        fs += "    vec2 position;\n";
        fs += "    float radius;\n";
        fs += "    float intensity;\n";
        fs += "};\n";
        fs += "uniform Light lights[16];\n";
        fs += "uniform int numLights;\n";
    }
    
    fs += "\nvoid main() {\n";
    fs += "    vec4 texColor = texture(texture0, fragTexCoord);\n";
    fs += "    vec4 resultColor = texColor * fragColor;\n";
    
    if (def.type == "Lit") {
        fs += "    vec4 lighting = vec4(0.1, 0.1, 0.1, 1.0);\n"; // Ambient floor
        fs += "    for(int i = 0; i < numLights; i++) {\n";
        fs += "        if (lights[i].type == 0) {\n"; // Point
        fs += "            float dist = distance(fragWorldPos, lights[i].position);\n";
        fs += "            float att = smoothstep(lights[i].radius, 0.0, dist);\n";
        fs += "            lighting.rgb += lights[i].color.rgb * lights[i].intensity * att;\n";
        fs += "        }\n";
        fs += "    }\n";
        fs += "    resultColor.rgb *= lighting.rgb;\n";
    }
    
    if (!def.fragmentLogic.empty()) {
        fs += def.fragmentLogic + "\n";
    }
    
    fs += "    finalColor = resultColor;\n";
    fs += "}\n";
    
    return fs;
}
