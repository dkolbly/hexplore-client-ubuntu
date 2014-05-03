#version 120            // --*-c-*--

attribute vec3 vertexPosition_modelspace;

varying float fragmentAmbient;

// constant for the entire mesh
uniform mat4 MVP;

void main(void) {
  vec4 v = vec4(vertexPosition_modelspace, 1);  // make it homogenous
  gl_Position = MVP * v;
}
