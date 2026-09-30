#version 120

attribute vec3 Position;
attribute vec3 Normal;
attribute vec2 Texcoord;

uniform mat4 worldMatrix;
uniform mat4 normalMatrix;
uniform mat4 MVP;

varying vec3 vPosWS;
varying vec3 vNormalWS;
varying vec2 vTexcoord;

void main()
{
    gl_Position = MVP * vec4(Position, 1.0);
    vPosWS = vec3(worldMatrix * vec4(Position, 1.0));
    vNormalWS = normalize(mat3(normalMatrix) * Normal);
    vTexcoord = Texcoord;
}
