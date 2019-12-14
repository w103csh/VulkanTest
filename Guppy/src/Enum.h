/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef ENUM_H
#define ENUM_H

enum class GAME_KEY {
    // VIRTUAL KEYS
    KEY_SHUTDOWN,
    // PHYSICAL KEYS
    KEY_UNKNOWN,
    KEY_ESC,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_SPACE,
    KEY_TAB,
    KEY_F,
    KEY_W,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_E,
    KEY_Q,
    KEY_O,
    KEY_P,
    KEY_K,
    KEY_L,
    KEY_I,
    // BRACKET/BRACE KEYS
    KEY_LEFT_BRACKET,
    KEY_RIGHT_BRACKET,
    // NUMBER KEYS
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUALS,
    KEY_BACKSPACE,
    // FUNCTION KEYS
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    // MODIFIERS
    KEY_ALT,
    KEY_CTRL,
    KEY_SHFT,
};

// clang-format off
using GameButtonBits = uint16_t;
typedef enum GAME_BUTTON : GameButtonBits {
    // These are 1 to 1 the constants for XInput
    DPAD_UP         = 0x0001,
    DPAD_DOWN       = 0x0002,
    DPAD_LEFT       = 0x0004,
    DPAD_RIGHT      = 0x0008,
    START           = 0x0010,
    BACK            = 0x0020,
    LEFT_THUMB      = 0x0040,
    RIGHT_THUMB     = 0x0080,
    LEFT_SHOULDER   = 0x0100,
    RIGHT_SHOULDER  = 0x0200,
    A               = 0x1000,
    B               = 0x2000,
    X               = 0x4000,
    Y               = 0x8000,
} GAME_BUTTON;
// clang-format on

enum class MODEL_FILE_TYPE {
    //
    UNKNOWN = 0,
    OBJ,
};

// clang-format off
typedef enum STATUS {
    READY =                 0x00000000,
    PENDING_VERTICES =      0x00000001,
    PENDING_BUFFERS =       0x00000002,
    PENDING_MATERIAL =      0x00000004,
    PENDING_TEXTURE =       0x00000008,
    PENDING_PIPELINE =      0x00000010,
    PENDING_SWAPCHAIN =     0x00000020,
    PENDING_MESH =          0x00000040,
    REDRAW =                0x00000100,
    UPDATE_BUFFERS =        0x00001000,
    DESTROYED =             0x00010000,
    UNKNOWN =               0x00020000,
    PENDING =               0x7FFFFFFF,
} STATUS;
// clang-format on

enum class VERTEX {
    DONT_CARE = -1,
    //
    COLOR = 0,
    TEXTURE,
    // This is TEXTURE with different attribute descriptions.
    SCREEN_QUAD,
};

enum class PUSH_CONSTANT {
    DONT_CARE = -1,
    //
    DEFAULT = 0,
    PBR,
    POST_PROCESS,
    DEFERRED,
    PRTCL_EULER,
    HFF_COLUMN,
};

enum class INPUT_ACTION {
    UP,
    DOWN,
    DBLCLK,
    UNKNOWN,
};

enum class MESH {
    DONT_CARE = -1,
    //
    COLOR = 0,
    LINE,
    TEXTURE,
};

// Add new to either RenderPass::ALL & Compute::ALL
enum class PASS : uint32_t {  // TODO: make this a bitmask
    // SWAPCHAIN
    DEFAULT = 0,
    IMGUI,
    // SAMPLER
    SAMPLER_DEFAULT,
    SAMPLER_PROJECT,
    // SCREEN_SPACE
    SCREEN_SPACE,
    SCREEN_SPACE_HDR_LOG,
    SCREEN_SPACE_BRIGHT,
    SCREEN_SPACE_BLUR_A,
    SCREEN_SPACE_BLUR_B,
    // COMPUTE
    COMPUTE_POST_PROCESS,
    // DEFERRED
    DEFERRED,
    // SHADOW
    SHADOW,
    //
    SKYBOX_NIGHT,
    // Used to indicate "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
};

enum class COMBINED_SAMPLER {
    MATERIAL,
    PIPELINE,
    PIPELINE_DEPTH,
    //
    DONT_CARE,
};

enum class STORAGE_IMAGE {
    PIPELINE,
    SWAPCHAIN,
    //
    DONT_CARE,
};

enum class STORAGE_BUFFER {
    POST_PROCESS,
    //
    DONT_CARE,
};

enum class STORAGE_BUFFER_DYNAMIC {
    PRTCL_EULER,
    PRTCL_POSITION,
    PRTCL_VELOCITY,
    PRTCL_NORMAL,
    //
    NORMAL,
    //
    DONT_CARE,
    VERTEX,  // Buffer usage only
};

enum class UNIFORM {
    // CAMERA
    CAMERA_PERSPECTIVE_DEFAULT,
    CAMERA_PERSPECTIVE_CUBE_MAP,
    // LIGHT
    LIGHT_DIRECTIONAL_DEFAULT,
    LIGHT_POSITIONAL_DEFAULT,
    LIGHT_SPOT_DEFAULT,
    LIGHT_POSITIONAL_PBR,
    LIGHT_POSITIONAL_SHADOW,
    LIGHT_CUBE_SHADOW,
    // GENERAL
    OBJ3D,
    FOG_DEFAULT,
    PROJECTOR_DEFAULT,
    SCREEN_SPACE_DEFAULT,
    DEFERRED_SSAO,
    SHADOW_DATA,
    // TESSELLATION
    TESSELLATION_DEFAULT,
    // GEOMETRY
    GEOMETRY_DEFAULT,
    // PARTICLE
    PRTCL_WAVE,
    //
    DONT_CARE,
};

enum class UNIFORM_DYNAMIC {
    // MATERIAL
    MATERIAL_DEFAULT,
    MATERIAL_PBR,
    MATERIAL_OBJ3D,
    // PARTICLE
    PRTCL_FOUNTAIN,
    PRTCL_ATTRACTOR,
    PRTCL_CLOTH,
    //
    MATRIX_4,
    // WATER
    HFF,
    //
    DONT_CARE,
};

enum class INPUT_ATTACHMENT {
    POSITION,
    NORMAL,
    COLOR,
    FLAGS,
    SSAO,
    //
    DONT_CARE,
};

enum class QUEUE {
    GRAPHICS,
    PRESENT,
    TRANSFER,
    COMPUTE,
};

enum class SCENE {
    DEFAULT,
    DEFERRED,
};

#endif  // !ENUM_H