#version 440

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D texY;
layout(binding = 2) uniform sampler2D texUV;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float opacity;
};

void main() {
    float y = texture(texY, vTexCoord).r;
    float u = texture(texUV, vTexCoord).r - 0.5;
    float v = texture(texUV, vTexCoord).g - 0.5;

    // BT.709
    float r = y + 1.5748 * v;
    float g = y - 0.1873 * u - 0.4681 * v;
    float b = y + 1.8556 * u;

    fragColor = vec4(r, g, b, 1.0) * opacity;
}
