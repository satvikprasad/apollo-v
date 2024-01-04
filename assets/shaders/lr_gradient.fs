#version 330

uniform vec4 prevColor;

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
  float t = fragTexCoord.x*fragTexCoord.x;
  vec4 c = mix(prevColor, fragColor, t);

  finalColor = c;
}
