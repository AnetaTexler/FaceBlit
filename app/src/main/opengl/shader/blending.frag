#version 150

precision highp float;

uniform sampler2D A;
uniform sampler2D B;
uniform sampler2D mask;
in vec2 texCoord_v;
out vec4 fragColor;

void main() {
  //fragColor = vec4(0.0f,1.0f,0.0f,1.0f);
  //fragColor = texture(A, texCoord_v);
  float alpha = texture(mask, texCoord_v).r;
  fragColor = alpha * texture(A, texCoord_v) + (1.0f - alpha) * texture(B, texCoord_v);
}