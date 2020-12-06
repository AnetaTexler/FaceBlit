#version 150

precision highp float;

uniform sampler2D texSampler;
in vec2 texCoord_v;
out vec4 fragColor;

void main() {
  //fragColor = vec4(0.0f,1.0f,0.0f,1.0f);
  fragColor = texture(texSampler, texCoord_v);
}