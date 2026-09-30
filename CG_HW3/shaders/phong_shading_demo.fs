#version 120

varying vec3 vPosWS;
varying vec3 vNormalWS;
varying vec2 vTexcoord;

uniform sampler2D mapKd;
uniform int hasMapKd;
uniform vec3 dirLightDir;
uniform vec3 dirLightRadiance;
uniform vec3 pointLightPos;
uniform vec3 pointLightIntensity;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform vec3 spotLightIntensity;
uniform float spotLightCutoffStart;
uniform float spotLightCutoffTotal;
uniform vec3 ambientLight;
uniform vec3 cameraPos;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;

void main()
{
    vec3 diffuseKd = Kd;
    if (hasMapKd == 1) {
        diffuseKd = texture2D(mapKd, vTexcoord).rgb;
    }

    vec3 N = normalize(vNormalWS);
    vec3 V = normalize(cameraPos - vPosWS);

    vec3 color = diffuseKd * ambientLight;

    // Directional light (Blinn-Phong).
    vec3 Ld = normalize(-dirLightDir);
    float diffD = max(dot(N, Ld), 0.0);
    vec3 Hd = normalize(Ld + V);
    float specD = pow(max(dot(N, Hd), 0.0), Ns);
    color += (diffuseKd * diffD + Ks * specD) * dirLightRadiance;

    // Point light (Blinn-Phong + attenuation).
    vec3 Lp = normalize(pointLightPos - vPosWS);
    float diffP = max(dot(N, Lp), 0.0);
    vec3 Hp = normalize(Lp + V);
    float specP = pow(max(dot(N, Hp), 0.0), Ns);
    float distP = max(length(pointLightPos - vPosWS), 0.001);
    float attP = 1.0 / (distP * distP);
    color += attP * (diffuseKd * diffP + Ks * specP) * pointLightIntensity;

    // Spot light (Blinn-Phong + attenuation + linear cone falloff).
    vec3 toLight = spotLightPos - vPosWS;      // point -> light
    vec3 lightToPoint = vPosWS - spotLightPos; // light -> point
    float distS = max(length(toLight), 0.001);
    vec3 L = normalize(toLight);
    vec3 Ls = normalize(lightToPoint);
    float diffS = max(dot(N, L), 0.0);
    vec3 Hs = normalize(L + V);
    float specS = pow(max(dot(N, Hs), 0.0), Ns);
    float attS = 1.0 / (distS * distS);

    float cosInner = cos(radians(spotLightCutoffStart));
    float cosOuter = cos(radians(spotLightCutoffTotal));
    float cosTheta = dot(normalize(spotLightDir), Ls);

    float spotFactor;
    if (cosTheta >= cosInner) {
        spotFactor = 1.0;
    } else if (cosTheta <= cosOuter) {
        spotFactor = 0.0;
    } else {
        spotFactor = (cosTheta - cosOuter) / (cosInner - cosOuter);
    }

    color += spotFactor * attS * (diffuseKd * diffS + Ks * specS) * spotLightIntensity;

    gl_FragColor = vec4(color, 1.0);
}
