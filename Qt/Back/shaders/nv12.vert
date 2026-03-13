#version 440

layout(location = 0) in vec4 aVertex;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 vTexCoord;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float opacity;
};

void main() {
    gl_Position = qt_Matrix * aVertex;
    vTexCoord = aTexCoord;
}
