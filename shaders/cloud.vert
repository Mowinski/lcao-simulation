#version 410 core
layout (location = 0) in vec4 aPos; // W is the valid flag

uniform mat4 uViewProj;

void main() {
    if (aPos.w < 0.5) {
        // Discard the point by moving it outside the clip space
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    } else {
        gl_Position = uViewProj * vec4(aPos.xyz, 1.0);
    }
    gl_PointSize = 2.0;
}
