#version 150

precision highp float;

uniform highp sampler2D stylePosGuide;
uniform highp sampler2D targetPosGuide;
uniform highp sampler2D styleAppGuide;
uniform highp sampler2D targetAppGuide;
uniform highp sampler2D styleImg;
uniform highp sampler2D jitterTable;
uniform highp usampler3D LUT;

// Only works when source and target are of the same size.
uniform int width;
uniform int height;

in vec2 texCoord_v;
out vec4 fragColor;

uniform float threshold;
float lambdaPos = 10.0; 
float lambdaApp = 1.2;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

bool inside(vec2 uv)
{
  return (all(greaterThanEqual(uv,vec2(0,0))) && all(lessThan(uv,vec2(width, height))));
}

float getTargetGuide(vec2 position) {
    vec2 pos_normalized = vec2( position.x / width, (float(position.y) / float(height)) );
    vec2 total_guide = lambdaPos * texture(targetPosGuide, pos_normalized).rg + lambdaApp * texture(targetAppGuide, pos_normalized).r;
    return (total_guide.x + total_guide.y);
}

float getStyleGuide(vec2 position) {
    vec2 pos_normalized = vec2( position.x / width, (float(position.y) / float(height)) );
    vec2 total_guide =  lambdaPos * texture(stylePosGuide, pos_normalized).rg + lambdaApp * texture(styleAppGuide, pos_normalized).r;
    return (total_guide.x + total_guide.y);
}

vec2 RandomJitterTable(vec2 uv)
{
  return (2*texture2D(jitterTable,((uv+vec2(0.5,0.5))/vec2(width, height))).xy)-vec2(1.0f,1.0f);
}

vec2 SeedPoint(vec2 p,float h)
{
  vec2 b = floor(p/h);
  // return floor(h*b); // Uncomment to disable jitter table.
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

float frac(float x) { return x-floor(x); }

vec4 pack(vec2 xy)
{
  float x = xy.x/255.0;
  float y = xy.y/255.0;
  return vec4(frac(x),floor(x)/255.0,
              frac(y),floor(y)/255.0);
}


void main() {
  
  vec2 p = gl_FragCoord.xy-vec2(0.5,0.5);
  vec2 o = lookUp(p);
  vec2 best_q = p;

  float e = 0;
  for(int level=6;level>=0;level--)
  {
    vec2 q = NearestSeed(p,pow(2.0,float(level)));
    vec2 u = lookUp(q);
    
    e = (abs(getTargetGuide(p)-getStyleGuide(u+(p-q))))*255.0;
    
    if (e<threshold)
    {
      best_q = q;
      o = u+(p-q); 
      if (inside(o)) { break; }
    }
  }

  // Uncomment to visualize chunks.
  /*fragColor = vec4(rand(best_q*3.0),rand(best_q*5.0),rand(best_q*7.0),1.0f);
  return;*/
 
  fragColor = pack(o);
 
}