#version 150

precision highp float;

#ifndef BLEND_RADIUS
  #define BLEND_RADIUS 100
#endif

uniform sampler2D A;
uniform sampler2D B;
uniform sampler2D mask;
uniform sampler2D facial_mask;
uniform sampler2D background;
uniform sampler2D inner_face_mask;
uniform sampler2D hair;
uniform sampler2D hair_mask;
in vec2 texCoord_v;
out vec4 fragColor;

uniform int width;
uniform int height;

float expandMask( sampler2D mask, int area ) {
	float sum = 0.0f;
	float weight = 0.0f;


	float mask_at_point = texture(mask, texCoord_v).r;

	if ( mask_at_point > 0.98f )
		return mask_at_point;

	float step_x = 1.0f / float(width);
	float step_y = 1.0f / float(height);

	for(int oy=-area;oy<=+area;oy++)
	  for(int ox=-area;ox<=+area;ox++)
		{
			vec2 displace = vec2( step_x * float(ox), step_y * float(oy) );
			sum += texture(mask, texCoord_v + displace).r;
			weight += 1.0;
		}
	return cos( (1.0f - (sum / weight)) * 3.14 / 2.0 );
}

void main() {
  //fragColor = vec4(0.0f,1.0f,0.0f,1.0f);
  //fragColor = texture(A, texCoord_v);
  
  float hair_alpha = max(texture(hair_mask, texCoord_v).r - texture(inner_face_mask, texCoord_v).r, 0.0f);
  float facial_mask = max(expandMask(facial_mask, 20) - hair_alpha, 0.0f);
  // float facial_mask = max(texture(facial_mask, texCoord_v).r - hair_alpha, 0.0f);

  /*fragColor = vec4(facial_mask,facial_mask,facial_mask,1.0f);
  return;*/

  float body_alpha = max(texture(mask, texCoord_v).r - facial_mask, 0.0f);
  float remained = 1.0f - (facial_mask + body_alpha + hair_alpha);
  /*fragColor = vec4(body_alpha + hair_alpha,body_alpha + hair_alpha,body_alpha + hair_alpha,1.0f);
  return;*/

  fragColor = body_alpha * texture(A, texCoord_v) + facial_mask * texture(B, texCoord_v) + hair_alpha * texture(hair, texCoord_v) + remained * texture(background, texCoord_v);
}