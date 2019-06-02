
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _DS_PRJ_DEF -1

#if _DS_PRJ_DEF > -1
layout(set=_DS_PRJ_DEF, binding=1) uniform DefaultProjector {
    mat4 matrix;
} projector;

layout(location=6) out vec4 fragProjTexCoord;
layout(location=7) out vec4 fragTest;

void setProjectorTexCoord(const in vec4 pos) {
    fragTest = pos;
    fragProjTexCoord = projector.matrix * pos;
}
#else
void setProjectorTexCoord(const in vec4 pos) { return; }
#endif