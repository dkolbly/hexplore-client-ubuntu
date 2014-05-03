#version 120            // --*-c-*--

attribute vec3 vertexPosition_modelspace;
attribute vec2 vertexUV;
attribute float vertexAmbient;
// WIP in vec3 vertexNormal;

// we are just sending along the color info to the fragment
// shader; our cardinality is vertices, so to cover the gap
// from vertices to fragments it gets interpolated (automatically?)
//out vec4 fragmentColor; 

varying vec2 fragmentUV;
varying float fragmentAmbient;

// constant for the entire mesh
uniform mat4 MVP;

// WIP...
/*
uniform vec4 LightPosition;
uniform vec3 LightIntensity;
uniform vec3 Kd;                // Diffuse reflectivity
uniform mat4 ModelViewMatrix;
uniform mat4 NormalMatrix;
uniform mat4 ProjectionMatrix;
*/

void main(void) {
  //vec3 tnorm = normalize(NormalMatrix * VertexNormal);
  
  vec4 v = vec4(vertexPosition_modelspace, 1);  // make it homogenous
  gl_Position = MVP * v;
  fragmentUV = vertexUV;
  fragmentAmbient = vertexAmbient;
}
