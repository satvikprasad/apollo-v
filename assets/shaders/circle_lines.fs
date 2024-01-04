#version 330

uniform float size;
uniform float width;

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
  float r = 0.5;
  vec2 len = fragTexCoord - vec2(0.5);
  float s1 = length(len) - r;
  float s2 = length(len) - r + width/size;

  if (s1 <= 0 && s2 >= 0) {
    finalColor = fragColor;
  } else {
    finalColor = vec4(0);
  }
}
