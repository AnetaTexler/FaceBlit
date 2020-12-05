#version 150

precision highp float;

uniform highp sampler2D stylePosGuide;
uniform highp sampler2D targetPosGuide;
uniform highp sampler2D styleAppGuide;
uniform highp sampler2D targetAppGuide;
uniform highp sampler2D styleImg;
uniform highp sampler2D jitterTable;
uniform usampler3D LUT;

// Only works when source and target are of the same size.
uniform int width;
uniform int height;

in vec2 texCoord_v;
out vec4 fragColor;

float threshold = 50.0f/255.0f;
int lambdaPos = 10; 
int lambdaApp = 2;
/*
bool inside(vec2 uv)
{
  return (all(greaterThanEqual(uv,vec2(0,0))) && all(lessThan(uv,vec2(width, height))));
}

float getTargetGuide(vec2 position) {
    vec2 pos_normalized = vec2( position.x / width, 1.0 - (float(position.y) / float(height)) );
    vec3 total_guide = lambdaPos * texture(targetPosGuide, pos_normalized).rgb + lambdaApp * texture(targetAppGuide, pos_normalized).rgb;
    return (total_guide.x + total_guide.y + total_guide.z);
}

float getStyleGuide(vec2 position) {
    vec2 pos_normalized = vec2( position.x / width, 1.0 - (float(position.y) / float(height)) );
    vec3 total_guide =  lambdaPos * texture(stylePosGuide, pos_normalized).rgb + lambdaApp * texture(styleAppGuide, pos_normalized).rgb;
    return (total_guide.x + total_guide.y + total_guide.z);
}

vec2 RandomJitterTable(vec2 uv)
{
  return (2*texture2D(jitterTable,((uv+vec2(0.5,0.5))/vec2(width, height))).xy)-vec2(1.0f,1.0f);
}

vec2 SeedPoint(vec2 p,float h)
{
  vec2 b = floor(p/h);
  vec2 j = RandomJitterTable(b);  
  return floor(h*(b+j));
}

vec2 NearestSeed(vec2 p,float h)
{
  vec2 s_nearest = vec2(0,0);
  float d_nearest = 10000.0;

  for(int x=-1;x<=+1;x++)
  for(int y=-1;y<=+1;y++)
  {
    vec2 s = SeedPoint(p+h*vec2(x,y),h);
    float d = length(s-p);
    if (d<d_nearest)
    {
      s_nearest = s;
      d_nearest = d;     
    }
  }

  return s_nearest;
}

vec2 lookUp( vec2 position ) {
    vec2 pos_normalized = vec2( float(position.x) / float(width), (float(position.y) / float(height)) );
    vec2 target_guide = texture(targetPosGuide, pos_normalized).rg;
    float target_app = texture(targetAppGuide, pos_normalized).r;
    vec2 tex = texture(LUT, vec3(target_app, target_guide.g, target_guide.r) ).rg;
    tex = vec2(tex.r, height - tex.g);
    return tex;
}
*/
void main() {
    /*fragColor = texture(stylePosGuide, texCoord_v);
    return;*/
/*
  vec2 p = gl_FragCoord.xy-vec2(0.5,0.5);
  vec2 o = lookUp(p);
*/
  /*for(int level=6;level>=0;level--)
  {
    vec2 q = NearestSeed(p,pow(2.0,float(level)));
    vec2 u = lookUp(q);
    
    float e = (abs(getTargetGuide(p)-getStyleGuide(u+(p-q))))*255.0;
    
    if (e<threshold)
    {
      o = u+(p-q); if (inside(o)) { break; }
    }
  }*/

  // fragColor = texture(styleAppGuide, texCoord_v);
  //  vec2 final_normalized = vec2( o.x / float(width), o.y / float(height) );
  // fragColor = vec4(final_normalized,0.0f,1.0f);
  //  fragColor = texture(styleImg, final_normalized);
  // gl_FragColor = pack(o);
  // fragColor = vec4(0.0f,1.0f,0.0f,1.0f);
  // fragColor = vec4(lookUp(p) / 1024,0.0f,1.0f);
  // vec2 lut_norm = vec2( lookUp(p).x / 728, lookUp(p).y / 1024 );
  // fragColor = texture(styleImg, lut_norm);

  // fragColor = vec4(0.0f,texture(jitterTable, texCoord_v).g,0.0f,1.0f);
  
 
  vec2 guide_pos = texture(stylePosGuide, texCoord_v).rg;
  guide_pos = vec2(guide_pos.r, 1.0f - guide_pos.g);
  fragColor = texture(styleImg, guide_pos);
  // fragColor = vec4(abs(guide_pos - texCoord_v)*100.0f,0.0f,1.0f);
}