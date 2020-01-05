/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DEF 0

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;
// OUT
layout(location=0) out vec3 outPosition;    // (world space)

void main() {
    const mat4 view = mat4(mat3(camera.view));
    outPosition = (inModel * vec4(inPosition, 1.0)).xyz;
    vec4 pos = camera.projection * view * vec4(outPosition, 1.0);
    gl_Position = vec4(pos.xyw, pos.w + 0.00001);
}
