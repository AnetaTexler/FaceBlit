#version 150

in vec2 in_Position;
in vec2 in_Texcoord;
out vec2 texCoord_v;

void main() {
  //texCoord_v = (in_Position)/2.0f + vec2(0.5,0.5);
  texCoord_v = in_Texcoord;
  gl_Position = vec4(in_Position,0.1f, 1.0f);
}