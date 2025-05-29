#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;
layout(push_constant) uniform PushConstants {
    vec4 color;
} pushConstants;

void main() {
    float alpha = texture(texSampler, fragUV).a;
    outColor = vec4(pushConstants.color.rgb, pushConstants.color.a * alpha);
}
