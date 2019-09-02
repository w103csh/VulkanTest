
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _DS_UNI_DEF 0

// DECLARATIONS
void setProjectorTexCoord(const in vec4 pos);

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// // PUSH CONSTANTS
// layout(push_constant) uniform PushBlock {
//     mat4 model;
// } pushConstantsBlock;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;
// OUT
layout(location=0) out vec3 fragPosition;
layout(location=1) out vec3 fragNormal;
layout(location=2) out vec4 fragColor;
// layout(location=3) out int fragVertexIndex;

void main() {
    // This obviously can be much more efficient. (Can it??)
    mat4 viewModel = camera.view * inModel;

    fragPosition = (viewModel * vec4(inPosition, 1.0)).xyz;
    fragNormal = normalize(mat3(viewModel) * inNormal);
    fragColor = inColor;
    
    vec4 pos = inModel * vec4(inPosition, 1.0);
    setProjectorTexCoord(pos);

    gl_Position = camera.viewProjection * pos;
    // fragVertexIndex = gl_VertexIndex;
}
