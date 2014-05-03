#version 120            // --*-c-*--

attribute vec3 vertexPosition_modelspace;
attribute vec2 vertexUV;
attribute float vertexAmbient;

// we are just sending along the color info to the fragment
// shader; our cardinality is vertices, so to cover the gap
// from vertices to fragments it gets interpolated (automatically?)
//out vec4 fragmentColor; 

varying vec2 fragmentUV;
varying float fragmentAmbient;
varying float fragmentFog;
varying vec3 worldPosn;

// constant for the entire mesh
uniform mat4 MVP;
uniform float fogDensity;

void main(void) {

  //const float fogDensity = fogDensity;
  const float LOG2 = 1.442695;
  
  vec4 v = vec4(vertexPosition_modelspace, 1);  // make it homogenous
  worldPosn = vertexPosition_modelspace;
  gl_Position = MVP * v;
  fragmentUV = vertexUV;
  fragmentAmbient = vertexAmbient;
  
  float atten = 1.0 / (1.0 + 0.001 * dot(gl_Position, gl_Position));
  //const float EXP_SCALE = -0.1 * 1.442695;
  const float EXP_SCALE = -1;
  float f = length(gl_Position);

  fragmentFog = clamp(exp2( - fogDensity * fogDensity * LOG2 * f * f ),
                      0.0,
                      1.0);
}
