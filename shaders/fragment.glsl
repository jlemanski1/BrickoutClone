#version 120

uniform vec3 colour;    // [0]: R, [1]:G, [2]:B

void main(void) {
    gl_FragColor = vec4(colour, 1.0);
}