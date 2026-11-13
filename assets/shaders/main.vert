#version 450

layout(location = 0) out vec2 vUV;

void main() {
    // Vertices sit at (0,0), (2,0), (0,2). The clipped portion is the screen, and UVs land on 0..1.
    vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
