#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
  vec2 coord = (gl_PointCoord - vec2(0.5))*2.0;
  float r = length(coord);
  float alpha = r <= 0.5 ? 1.0 : mix(0.0, 1.0, 1.0 - (r - 0.5)*2.0);
  outColor   = vec4(fragColor*alpha, alpha);
}
