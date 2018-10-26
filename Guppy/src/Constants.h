
#pragma once

#include <glm/glm.hpp>
#include <map>
#include <util.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "Vertex.h"

typedef uint32_t Flags;

constexpr auto APP_SHORT_NAME = "Guppy Application";
constexpr auto DEPTH_PRESENT = true;
const std::string ROOT_PATH = "..\\..\\..\\";  // TODO: this should come from something meaningful ...

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

/* Number of viewports and number of scissors have to be the same */
/* at pipeline creation and in any call to set them dynamically   */
/* They also have to be the same as each other                    */
#define NUM_VIEWPORTS 1
#define NUM_SCISSORS NUM_VIEWPORTS

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_LUNARG_standard_validation",
};

const std::vector<const char*> INSTANCE_EXTENSIONS = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
    // VK_KHR_SWAPCHAIN_EXTENSION_NAME, in init_swapchain_extension now
};

// const VkClearColorValue CLEAR_VALUE = {0.5f, 0.5f, 0.5f, 0.5f};
const VkClearColorValue CLEAR_VALUE = {};

// Type for the vertex buffer indices (this is also used in vkCmdBindIndexBuffer)
typedef uint32_t VB_INDEX_TYPE;

// Application wide up vector
static glm::vec3 UP_VECTOR = glm::vec3(0.0f, 0.0f, 1.0f);
// static glm::vec3 UP_VECTOR = glm::vec3(0.0f, 0.0f, 1.0f);

const auto ENABLE_SAMPLE_SHADING = VK_TRUE;
const float MIN_SAMPLE_SHADING = 0.2f;

// objs
const std::string CHALET_MODEL_PATH = ROOT_PATH + "data\\chalet.obj";
const std::string MED_H_MODEL_PATH = ROOT_PATH + "data\\Medieval_House.obj";
const std::string SPHERE_MODEL_PATH = ROOT_PATH + "data\\sphere.obj";
// jpgs
const std::string CHALET_TEX_PATH = ROOT_PATH + "images\\chalet.jpg";
const std::string STATUE_TEX_PATH = ROOT_PATH + "images\\texture.jpg";
const std::string VULKAN_TEX_PATH = ROOT_PATH + "images\\vulkan.png";
const std::string MED_H_DIFF_TEX_PATH = ROOT_PATH + "images\\Medieval_House\\Medieval_House_Diff.png";
const std::string MED_H_NORM_TEX_PATH = ROOT_PATH + "images\\Medieval_House\\Medieval_House_Nor.png";
const std::string MED_H_SPEC_TEX_PATH = ROOT_PATH + "images\\Medieval_House\\Medieval_House_Spec.png";
