/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

// DECLARATIONS
vec3 blinnPhongShade();
void setColorDefaults();
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
// layout(location=3) in flat int inVertexIndex;
// OUT
layout(location=0) out vec4 outColor;

// GLOBAL
vec3    Ka, // ambient coefficient
        Kd, // diffuse coefficient
        Ks, // specular coefficient
        n;  // normal
float opacity;

vec3 transform(vec3 v) { return v; }

void main() {
    setColorDefaults();
    outColor = gammaCorrect(blinnPhongShade(), opacity);
}