#version 120

uniform vec3 colour;

void main(void) {
    gl_FragColor = vec4(colour, 1.0);
}