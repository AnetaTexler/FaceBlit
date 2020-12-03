#version 150

in vec2 in_Position;
out vec2 texCoord_v;

void main() {
  texCoord_v = (in_Position)/2.0f + vec2(0.5,0.5);
  gl_Position = vec4(in_Position,0.1f, 1.0f);
}