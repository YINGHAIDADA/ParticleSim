#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTex;
layout(push_constant) uniform PushConstants {
    float panelU;
    float zoom;
    vec2 center;
    float toolbarV;
} pc;

vec2 sceneUV(vec2 screenUV) {
    if (screenUV.x < pc.panelU) return screenUV;
    float canvas = max(1.0 - pc.panelU, 0.0001);
    float localX = (screenUV.x - pc.panelU) / canvas;
    float zoom = max(pc.zoom, 0.05);
    return vec2(
        pc.center.x + (localX - 0.5) * canvas / zoom,
        pc.center.y + (screenUV.y - 0.5) / zoom
    );
}

void main() {
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 uv = sceneUV(vUV);
    if (vUV.x < pc.panelU || vUV.y < pc.toolbarV || uv.x < pc.panelU || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return;
    }

    vec3 texColor = texture(uTex, uv).rgb;
    float brightness = dot(texColor, vec3(0.2126, 0.7152, 0.0722));
    if (texColor.b < 0.22 && brightness > 0.30) {
        outColor = vec4(texColor, 1.0);
    }
}
