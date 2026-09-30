#version 120

varying vec2 iTexCoord;

// Material properties.
uniform sampler2D mapKd;

void main()
{
    // We create the uv coordinate from the top, so don't need to inverse.
    gl_FragColor = texture2D(mapKd, vec2(iTexCoord.x, 1.0 - iTexCoord.y));
}
