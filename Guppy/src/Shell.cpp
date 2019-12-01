/*
 * Modifications copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * -------------------------------
 * Copyright (C) 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Shell.h"

#include <cassert>
#include <array>
#include <iostream>
#include <string>
#include <sstream>
#include <set>

#include "EventHandlers.h"
#include "Game.h"
#include "Helpers.h"
#include "InputHandler.h"
// HANDLERS
#include "InputHandler.h"
#include "SoundHandler.h"

Shell::Shell(Game &game, Handlers &&handlers)
    : limitFramerate(true),
      framesPerSecondLimit(10),
      game_(game),
      settings_(game.settings()),
      instanceExtensions_{
          VK_KHR_SURFACE_EXTENSION_NAME,  // require generic WSI extensions
      },
      /** Device extensions:
       *    If an extension is required to run flip that flag and the program will not start without it. If you want the
       * extension to be optional set the required flag to false. If you do not want the extension to try to be enabled set
       * the last flag and the extension will not be enabled even if its possible.
       */
      deviceExtensionInfo_{
          {VK_KHR_SWAPCHAIN_EXTENSION_NAME, true, true},
          {VK_EXT_DEBUG_MARKER_EXTENSION_NAME, false, settings_.try_debug_markers},
          {VK_EXT_DEBUG_MARKER_EXTENSION_NAME, false, settings_.try_debug_markers},
          {VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME, false, false},
          {VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME, false, false},
      },
      currentTime_(0.0),
      elapsedTime_(0.0),
      handlers_(std::move(handlers)),
      ctx_(),
      gameTick_(1.0f / settings_.ticks_per_second),
      gameTime_(gameTick_),
      debugUtilsMessenger_(VK_NULL_HANDLE) {
    /**
     * Leaving this validation layer code in but I am going to start using vkconfig.exe instead. It does everything
     * the layer code here already did but better. You can read about it here
     * https://vulkan.lunarg.com/doc/sdk/1.1.126.0/windows/vkconfig.html in case you forget how to use it. I feel like if
     * LunarG made the app then they are probably willing to maintain it which is easier than figuring out how the layers
     * will change over time. Its also configurable through the UI so it really is a nice little thing to use.
     *
     * Things to note:
     *
     * - They aparently removed the verbose oject tracking when they moved towards VK_LAYER_KHRONOS_validation from
     * VK_LAYER_LUNARG_standard_validation which is a huge bummer.
     *
     * - If I set the info flag for VK_LAYER_KHRONOS_validation I was not recieving the same messages as enabling it
     * programmatically. Fortunately, the message were only for device extension enabling which is not super useful, or has
     * not been thus far. It does make we wonder if there are other messages I am potentially missing though.
     */
    if (settings_.validate_verbose) assert(settings_.validate);
    if (settings_.validate) {
        instanceLayers_.push_back("VK_LAYER_KHRONOS_validation");
        instanceExtensions_.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
}

Shell::~Shell() = default;

void Shell::log(LogPriority priority, const char *msg) const {
    std::ostream &st = (priority >= LogPriority::LOG_ERR) ? std::cerr : std::cout;
    st << msg << std::endl;
}

void Shell::init() {
    // TODO: Add init functions here... inline ???
    handlers_.pSound->init();
}

void Shell::update(const double elapsed) {
    // TODO: Add update functions here... inline ???
    handlers_.pSound->update(elapsed);
    handlers_.pInput->updateInput(static_cast<float>(elapsed));  // remove me !!!!!
    handlers_.pInput->update(elapsed);
}

void Shell::destroy() {
    // TODO: Add destroy functions here... inline ???
    handlers_.pSound->destroy();
}

void Shell::initVk() {
    initInstance();

    ext::CreateInstanceEXTs(ctx_.instance, settings_.validate);
    initDebugUtilsMessenger();

    initPhysicalDev();
}

void Shell::cleanupVk() {
    if (settings_.validate) ext::vkDestroyDebugUtilsMessengerEXT(ctx_.instance, debugUtilsMessenger_, nullptr);
    destroyInstance();
}

void Shell::assertAllInstanceLayers() const {
    // Check if all instance layers required are present
    for (const auto &layerName : instanceLayers_) {
        auto it = std::find_if(layerProps_.begin(), layerProps_.end(), [&layerName](auto layer_prop) {
            return std::strcmp(layer_prop.properties.layerName, layerName) == 0;
        });
        if (it == layerProps_.end()) {
            std::stringstream ss;
            ss << "instance layer " << layerName << " is missing";
            log(LogPriority::LOG_WARN, ss.str().c_str());
            assert(false);
        }
    }
}

void Shell::assertAllInstanceExtensions() const {
    // Check if all instance extensions required are present
    for (const auto &extensionName : instanceExtensions_) {
        auto it = std::find_if(instExtProps_.begin(), instExtProps_.end(),
                               [&extensionName](auto ext) { return std::strcmp(extensionName, ext.extensionName) == 0; });
        if (it == instExtProps_.end()) {
            std::stringstream ss;
            ss << "instance extension " << extensionName << " is missing";
            throw std::runtime_error(ss.str());
        }
    }
}

bool Shell::hasAllDeviceLayers(VkPhysicalDevice phy) const {
    // Something here?

    return true;
}

bool Shell::hasAllDeviceExtensions(VkPhysicalDevice phy) const {
    //// enumerate device extensions
    // std::vector<VkExtensionProperties> exts;
    // vk::enumerate(phy, nullptr, exts);

    // std::set<std::string> ext_names;
    // for (const auto &ext : exts) ext_names.insert(ext.extensionName);

    //// all listed device extensions are required
    // for (const auto &name : device_extensions_) {
    //    if (ext_names.find(name) == ext_names.end()) return false;
    //}

    return true;
}

void Shell::initInstance() {
    // enumerate instance layer & extension properties
    enumerateInstanceProperties();

    assertAllInstanceLayers();
    assertAllInstanceExtensions();

    uint32_t version = VK_VERSION_1_0;
    determineApiVersion(version);

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = settings_.name.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Shell";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = version;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = nullptr;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = static_cast<uint32_t>(instanceLayers_.size());
    inst_info.ppEnabledLayerNames = instanceLayers_.data();
    inst_info.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions_.size());
    inst_info.ppEnabledExtensionNames = instanceExtensions_.data();

    VkResult res = vkCreateInstance(&inst_info, nullptr, &ctx_.instance);
    assert(res == VK_SUCCESS);
}

void Shell::initDebugUtilsMessenger() {
    if (!settings_.validate) return;

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    debugInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    if (settings_.validate_verbose) {
        debugInfo.messageSeverity |=
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        debugInfo.messageType |=
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    }

    debugInfo.pfnUserCallback = events::DebugUtilsMessenger;
    debugInfo.pUserData = this;

    vk::assert_success(ext::vkCreateDebugUtilsMessengerEXT(ctx_.instance, &debugInfo, nullptr, &debugUtilsMessenger_));
}

void Shell::initPhysicalDev() {
    enumeratePhysicalDevs();

    ctx_.physicalDev = VK_NULL_HANDLE;
    pickPhysicalDev();

    if (ctx_.physicalDev == VK_NULL_HANDLE) throw std::runtime_error("failed to find any capable Vulkan physical device");
}

void Shell::createContext() {
    createDev();

    ext::CreateDeviceEXTs(ctx_);
    initDevQueues();

    // TODO: should this do this?
    // initialize ctx_.{surface,format} before attach_shell
    createSwapchain();

    // need image count
    createBackBuffers();

    game_.attachShell(*this);
}

void Shell::destroyContext() {
    if (ctx_.dev == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(ctx_.dev);

    destroySwapchain();

    game_.detachShell();

    destroyBackBuffers();

    // ctx_.game_queue = VK_NULL_HANDLE;
    // ctx_.present_queue = VK_NULL_HANDLE;

    vkDeviceWaitIdle(ctx_.dev);
    vkDestroyDevice(ctx_.dev, nullptr);
    ctx_.dev = VK_NULL_HANDLE;
}

void Shell::createDev() {
    // queue create info
    std::set<uint32_t> unique_queue_families = {
        ctx_.graphicsIndex,
        ctx_.presentIndex,
        ctx_.transferIndex,
        ctx_.computeIndex,
    };
    float queue_priorities[1] = {0.0f};  // only one queue per family atm
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    for (auto &index : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = nullptr;
        queue_info.queueCount = 1;  // only one queue per family atm
        queue_info.queueFamilyIndex = index;
        queue_info.pQueuePriorities = queue_priorities;
        queue_infos.push_back(queue_info);
    }

    // features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = ctx_.samplerAnisotropyEnabled;
    deviceFeatures.sampleRateShading = ctx_.sampleRateShadingEnabled;
    deviceFeatures.fragmentStoresAndAtomics = ctx_.computeShadingEnabled;
    deviceFeatures.tessellationShader = ctx_.tessellationShadingEnabled;
    deviceFeatures.geometryShader = ctx_.geometryShadingEnabled;
    deviceFeatures.fillModeNonSolid = ctx_.wireframeShadingEnabled;
    deviceFeatures.independentBlend = ctx_.independentBlendEnabled;

    // Phyiscal device extensions names
    auto &phyDevProps = ctx_.physicalDevProps[ctx_.physicalDevIndex];
    std::vector<const char *> enabledExtensionNames;
    for (const auto &extInfo : phyDevProps.phyDevExtInfos)
        if (extInfo.valid) enabledExtensionNames.push_back(extInfo.name);

    VkDeviceCreateInfo devInfo = {};
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    devInfo.pQueueCreateInfos = queue_infos.data();
    devInfo.pEnabledFeatures = &deviceFeatures;
    devInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size());
    devInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

    // Physical device extension featrues (For some reason using the name above doesn't always work)
    // void **pNext = &(const_cast<void *>(devInfo.pNext));
    if (ctx_.vertexAttributeDivisorEnabled) {
        assert(false);  // Never tested. Was using the name above sufficient?
        // *pNext = &phyDevProps.featVertAttrDiv;
        // pNext = &phyDevProps.featVertAttrDiv.pNext;
    }
    if (ctx_.transformFeedbackEnabled) {
        assert(false);
        // *pNext = &phyDevProps.featTransFback;
        // pNext = &phyDevProps.featTransFback.pNext;
    }

    vk::assert_success(vkCreateDevice(ctx_.physicalDev, &devInfo, nullptr, &ctx_.dev));
}

void Shell::createBackBuffers() {
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // BackBuffer is used to track which swapchain image and its associated
    // sync primitives are busy.  Having more BackBuffer's than swapchain
    // images may allow us to replace CPU wait on present_fence by GPU wait
    // on acquire_semaphore.
    const int count = ctx_.imageCount + 1;
    for (int i = 0; i < count; i++) {
        BackBuffer buf = {};
        vk::assert_success(vkCreateSemaphore(ctx_.dev, &sem_info, nullptr, &buf.acquireSemaphore));
        vk::assert_success(vkCreateSemaphore(ctx_.dev, &sem_info, nullptr, &buf.renderSemaphore));
        vk::assert_success(vkCreateFence(ctx_.dev, &fence_info, nullptr, &buf.presentFence));

        ctx_.backBuffers.push(buf);
    }
}

void Shell::destroyBackBuffers() {
    while (!ctx_.backBuffers.empty()) {
        const auto &buf = ctx_.backBuffers.front();

        vkDestroySemaphore(ctx_.dev, buf.acquireSemaphore, nullptr);
        vkDestroySemaphore(ctx_.dev, buf.renderSemaphore, nullptr);
        vkDestroyFence(ctx_.dev, buf.presentFence, nullptr);

        ctx_.backBuffers.pop();
    }
}

// TODO: this doesn't create the swapchain
void Shell::createSwapchain() {
    ctx_.surface = createSurface(ctx_.instance);

    VkBool32 supported;
    vk::assert_success(vkGetPhysicalDeviceSurfaceSupportKHR(ctx_.physicalDev, ctx_.presentIndex, ctx_.surface, &supported));
    assert(supported);
    // this should be guaranteed by the platform-specific can_present call
    supported = canPresent(ctx_.physicalDev, ctx_.presentIndex);
    assert(supported);

    enumerateSurfaceProperties();
    determineSwapchainSurfaceFormat();
    determineSwapchainPresentMode();
    determineSwapchainImageCount();
    determineDepthFormat();

    // suface dependent flags
    /*  TODO: Clean up what happens if the GPU doesn't handle linear blitting.
        Right now this flag just turns it off.

        There are two alternatives in this case. You could implement a function that searches common texture image
        formats for one that does support linear blitting, or you could implement the mipmap generation in software
        with a library like stb_image_resize. Each mip level can then be loaded into the image in the same way that
        you loaded the original image.
    */
    ctx_.linearBlittingSupported =
        helpers::findSupportedFormat(ctx_.physicalDev, {ctx_.surfaceFormat.format}, VK_IMAGE_TILING_OPTIMAL,
                                     VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != VK_FORMAT_UNDEFINED;

    // defer to resizeSwapchain()
    ctx_.swapchain = VK_NULL_HANDLE;
    // ctx_.extent.width = UINT32_MAX;
    // ctx_.extent.height = UINT32_MAX;
}

void Shell::destroySwapchain() {
    if (ctx_.swapchain != VK_NULL_HANDLE) {
        game_.detachSwapchain();

        vkDestroySwapchainKHR(ctx_.dev, ctx_.swapchain, nullptr);
        ctx_.swapchain = VK_NULL_HANDLE;
    }

    vkDestroySurfaceKHR(ctx_.instance, ctx_.surface, nullptr);
    ctx_.surface = VK_NULL_HANDLE;
}

void Shell::onKey(GAME_KEY key) {
    switch (key) {
        case GAME_KEY::KEY_MINUS: {
            if (framesPerSecondLimit > 1) framesPerSecondLimit--;
        } break;
        case GAME_KEY::KEY_EQUALS: {
            if (framesPerSecondLimit < UINT8_MAX) framesPerSecondLimit++;
        } break;
        case GAME_KEY::KEY_BACKSPACE: {
            limitFramerate = !limitFramerate;
        } break;
        default:;
    }
    game_.onKey(key);
}

void Shell::resizeSwapchain(uint32_t widthHint, uint32_t heightHint, bool refreshCapabilities) {
    if (determineSwapchainExtent(widthHint, heightHint, refreshCapabilities)) return;

    auto &caps = ctx_.surfaceProps.capabilities;

    VkSurfaceTransformFlagBitsKHR preTransform;  // ex: rotate 90
    if (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = caps.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (uint32_t i = 0; i < sizeof(compositeAlphaFlags); i++) {
        if (caps.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = ctx_.surface;
    swapchain_info.minImageCount = ctx_.imageCount;
    swapchain_info.imageFormat = ctx_.surfaceFormat.format;
    swapchain_info.imageColorSpace = ctx_.surfaceFormat.colorSpace;
    swapchain_info.imageExtent = ctx_.extent;
    swapchain_info.imageArrayLayers = 1;
    // TODO: Should these flags be pass setup dependent???
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // | VK_IMAGE_USAGE_STORAGE_BIT;
    swapchain_info.preTransform = caps.currentTransform;
    swapchain_info.compositeAlpha = compositeAlpha;
    swapchain_info.presentMode = ctx_.mode;
#ifndef __ANDROID__
    // Clip obscured pixels (by other windows for example). Only needed if the pixel info is reused.
    swapchain_info.clipped = true;
#else
    swapchain_info.clipped = false;
#endif
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 0;
    swapchain_info.pQueueFamilyIndices = nullptr;

    if (ctx_.graphicsIndex != ctx_.presentIndex) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between the
        // queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        uint32_t queue_families[2] = {ctx_.graphicsIndex, ctx_.presentIndex};
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = queue_families;
    }

    swapchain_info.oldSwapchain = ctx_.swapchain;

    vk::assert_success(vkCreateSwapchainKHR(ctx_.dev, &swapchain_info, nullptr, &ctx_.swapchain));

    // destroy the old swapchain
    if (swapchain_info.oldSwapchain != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx_.dev);

        game_.detachSwapchain();
        vkDestroySwapchainKHR(ctx_.dev, swapchain_info.oldSwapchain, nullptr);
    }

    game_.attachSwapchain();
}

void Shell::addGameTime(float time) {
    int max_ticks = 1;

    if (!settings_.no_tick) gameTime_ += time;

    while (gameTime_ >= gameTick_ && max_ticks--) {
        game_.onTick();
        gameTime_ -= gameTick_;
    }
}

void Shell::acquireBackBuffer() {
    auto &buf = ctx_.backBuffers.front();

    // wait until acquire and render semaphores are waited/unsignaled
    vk::assert_success(vkWaitForFences(ctx_.dev, 1, &buf.presentFence, true, UINT64_MAX));

    VkResult res = VK_TIMEOUT;  // Anything but VK_SUCCESS
    while (res != VK_SUCCESS) {
        res = vkAcquireNextImageKHR(ctx_.dev, ctx_.swapchain, UINT64_MAX, buf.acquireSemaphore, VK_NULL_HANDLE,
                                    &buf.imageIndex);
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            // Swapchain is out of date (e.g. the window was resized) and
            // must be recreated:
            resizeSwapchain(0, 0);  // width and height hints should be ignored
        } else {
            assert(!res);
        }
    }

    // reset the fence (AFTER POTENTIAL RECREATION OF SWAP CHAIN)
    vk::assert_success(vkResetFences(ctx_.dev, 1, &buf.presentFence));  // *

    ctx_.acquiredBackBuffer = buf;
    ctx_.backBuffers.pop();
}

void Shell::presentBackBuffer() {
    const auto &buf = ctx_.acquiredBackBuffer;

    if (!settings_.no_render) game_.onFrame(gameTime_ / gameTick_);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = (settings_.no_render) ? &buf.acquireSemaphore : &buf.renderSemaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &ctx_.swapchain;
    present_info.pImageIndices = &buf.imageIndex;

    VkResult res = vkQueuePresentKHR(ctx_.queues[ctx_.presentIndex], &present_info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        resizeSwapchain(0, 0);  // width and height hints should be ignored
    } else {
        assert(!res);
    }

    vk::assert_success(vkQueueSubmit(ctx_.queues[ctx_.presentIndex], 0, nullptr, buf.presentFence));
    ctx_.backBuffers.push(buf);
}

// ********
// NEW FUNCTIONS FROM MY OTHER INIT CODE
// ********

void Shell::initDevQueues() {
    std::set<uint32_t> unique_queue_families = {
        ctx_.graphicsIndex,
        ctx_.presentIndex,
        ctx_.transferIndex,
        ctx_.computeIndex,
    };
    for (auto queue_family_index : unique_queue_families) {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(ctx_.dev, queue_family_index, 0, &queue);
        assert(queue != VK_NULL_HANDLE);
        ctx_.queues[queue_family_index] = queue;
    }
}

void Shell::destroyInstance() { vkDestroyInstance(ctx_.instance, nullptr); }

void Shell::enumerateInstanceProperties() {
#ifdef __ANDROID__
    // This place is the first place for samples to use Vulkan APIs.
    // Here, we are going to open Vulkan.so on the device and retrieve function pointers using
    // vulkan_wrapper helper.
    if (!InitVulkan()) {
        LOGE("Failied initializing Vulkan APIs!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    LOGI("Loaded Vulkan APIs.");
#endif

    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for VkLayerProperties. The loader indicates that
     * by returning a VK_INCOMPLETE status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
     */
    uint32_t instance_layer_count;
    vk::assert_success(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

    std::vector<VkLayerProperties> props(instance_layer_count);
    vk::assert_success(vkEnumerateInstanceLayerProperties(&instance_layer_count, props.data()));

    /*
     * Now gather the extension list for each instance layer.
     */
    for (auto &prop : props) {
        layerProps_.push_back({prop});
        enumerateInstanceLayerExtensionProperties(layerProps_.back());
    }

    // Get instance extension properties
    enumerateInstanceExtensionProperties();
}

void Shell::enumerateInstanceLayerExtensionProperties(LayerProperties &layerProps) {
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count;
    VkResult res;
    char *layer_name = layerProps.properties.layerName;

    do {
        // This could be cleaner obviously
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, nullptr);
        if (res) return;
        if (instance_extension_count == 0) return;

        layerProps.extensionProps.resize(instance_extension_count);
        instance_extensions = layerProps.extensionProps.data();
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, instance_extensions);
    } while (res == VK_INCOMPLETE);
}

void Shell::enumerateDeviceLayerExtensionProperties(PhysicalDeviceProperties &props, LayerProperties &layerProps) {
    uint32_t extensionCount;
    std::vector<VkExtensionProperties> extensions;
    char *layer_name = layerProps.properties.layerName;

    vk::assert_success(vkEnumerateDeviceExtensionProperties(props.device, layer_name, &extensionCount, nullptr));

    extensions.resize(extensionCount);
    vk::assert_success(vkEnumerateDeviceExtensionProperties(props.device, layer_name, &extensionCount, extensions.data()));

    if (!extensions.empty())
        for (auto &ext : extensions)
            props.layerExtensionMap.insert(std::pair<const char *, VkExtensionProperties>(layer_name, ext));
}

void Shell::enumerateInstanceExtensionProperties() {
    uint32_t ext_count;
    vk::assert_success(vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr));
    instExtProps_.resize(ext_count);
    vk::assert_success(vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, instExtProps_.data()));
}

void Shell::enumerateSurfaceProperties() {
    // capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx_.physicalDev, ctx_.surface, &ctx_.surfaceProps.capabilities);

    // surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx_.physicalDev, ctx_.surface, &formatCount, NULL);
    if (formatCount != 0) {
        ctx_.surfaceProps.surfFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(ctx_.physicalDev, ctx_.surface, &formatCount,
                                             ctx_.surfaceProps.surfFormats.data());
    }

    // present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx_.physicalDev, ctx_.surface, &presentModeCount, NULL);
    if (presentModeCount != 0) {
        ctx_.surfaceProps.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx_.physicalDev, ctx_.surface, &presentModeCount,
                                                  ctx_.surfaceProps.presentModes.data());
    }
}

void Shell::enumeratePhysicalDevs(uint32_t physicalDevCount) {
    std::vector<VkPhysicalDevice> devs;
    uint32_t const req_count = physicalDevCount;

    VkResult res = vkEnumeratePhysicalDevices(ctx_.instance, &physicalDevCount, NULL);
    assert(res == VK_SUCCESS && physicalDevCount >= req_count);
    devs.resize(physicalDevCount);

    vk::assert_success(vkEnumeratePhysicalDevices(ctx_.instance, &physicalDevCount, devs.data()));

    for (auto &dev : devs) {
        PhysicalDeviceProperties props;
        props.device = dev;

        // Queue family properties
        vkGetPhysicalDeviceQueueFamilyProperties(props.device, &props.queueFamilyCount, NULL);
        assert(props.queueFamilyCount >= 1);

        props.queueProps.resize(props.queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(props.device, &props.queueFamilyCount, props.queueProps.data());
        assert(props.queueFamilyCount >= 1);

        // Memory properties
        vkGetPhysicalDeviceMemoryProperties(props.device, &props.memoryProperties);

        // Extension properties
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(props.device, NULL, &extensionCount, NULL);
        props.extensionProperties.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(dev, NULL, &extensionCount, props.extensionProperties.data());

        // This could all be faster, but I doubt it will make a significant difference at any point.
        for (const auto &extInfo : deviceExtensionInfo_) {
            auto it =
                std::find_if(props.extensionProperties.begin(), props.extensionProperties.end(),
                             [&extInfo](const auto &extProp) { return strcmp(extInfo.name, extProp.extensionName) == 0; });

            if (it == props.extensionProperties.end()) {
                // TODO: better logging
                if (extInfo.required) {
                    assert(false && "Couldn't find required extension");
                    exit(EXIT_FAILURE);
                }
                props.phyDevExtInfos.push_back(extInfo);
                props.phyDevExtInfos.back().valid = false;
            } else {
                props.phyDevExtInfos.push_back(extInfo);

                if (strcmp(extInfo.name, (char *)VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    props.phyDevExtInfos.back().valid = true;
                    continue;

                } else if (strcmp(extInfo.name, (char *)VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
                    if (extInfo.tryToEnabled) {
                        assert(false);  // TODO: what should this be ?? There are a bunch of extensions functions.
                    }

                } else if (strcmp(extInfo.name, (char *)VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME) == 0) {
                    if (extInfo.tryToEnabled) {
                        assert(false);  // Not tested.
                        VkPhysicalDeviceFeatures2 features = {};
                        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                        features.pNext = &props.featVertAttrDiv;
                        vkGetPhysicalDeviceFeatures2(props.device, &features);
                        // Check features
                        if (props.featVertAttrDiv.vertexAttributeInstanceRateDivisor) {
                            props.featVertAttrDiv.vertexAttributeInstanceRateZeroDivisor = VK_FALSE;
                            props.phyDevExtInfos.back().valid = true;
                            continue;
                        }
                    }

                } else if (strcmp(extInfo.name, (char *)VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME) == 0) {
                    if (extInfo.tryToEnabled) {
                        VkPhysicalDeviceFeatures2 features = {};
                        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                        features.pNext = &props.featTransFback;
                        vkGetPhysicalDeviceFeatures2(props.device, &features);
                        // Check features
                        if (props.featTransFback.transformFeedback) {
                            props.featTransFback.geometryStreams = VK_FALSE;
                            props.phyDevExtInfos.back().valid = true;
                            continue;
                        }
                    }

                } else {
                    assert(false && "Unhandled physical device extension");
                    exit(EXIT_FAILURE);
                }

                props.phyDevExtInfos.back().valid = false;
            }
        }

        // Physical device features
        vkGetPhysicalDeviceFeatures(props.device, &props.features);

        // TODO: enumerate extension properties.

        // Physical device properties
        vkGetPhysicalDeviceProperties(props.device, &props.properties);

        // Layer extension properties
        for (auto &layer_prop : layerProps_) {
            enumerateDeviceLayerExtensionProperties(props, layer_prop);
        }

        ctx_.physicalDevProps.push_back(props);
    }

    devs.erase(devs.begin(), devs.end());
}

void Shell::pickPhysicalDev() {
    // Iterate over each enumerated physical device
    for (size_t i = 0; i < ctx_.physicalDevProps.size(); i++) {
        auto &props = ctx_.physicalDevProps[i];
        ctx_.physicalDevIndex = ctx_.graphicsIndex = ctx_.presentIndex = ctx_.transferIndex = ctx_.computeIndex = UINT32_MAX;

        if (isDevSuitable(props, ctx_.graphicsIndex, ctx_.presentIndex, ctx_.transferIndex, ctx_.computeIndex)) {
            // We found a suitable physical device so set relevant data.
            ctx_.physicalDevIndex = i;
            ctx_.physicalDev = props.device;

            // Convenience variables
            ctx_.memProps = ctx_.physicalDevProps[ctx_.physicalDevIndex].memoryProperties;
            deviceExtensionInfo_ = props.phyDevExtInfos;

            // Flags
            for (const auto &extInfo : deviceExtensionInfo_) {
                if (strcmp(extInfo.name, (char *)VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
                    ctx_.debugMarkersEnabled = extInfo.valid;
                if (strcmp(extInfo.name, (char *)VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME) == 0)
                    ctx_.vertexAttributeDivisorEnabled = extInfo.valid;
                if (strcmp(extInfo.name, (char *)VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME) == 0)
                    ctx_.transformFeedbackEnabled = extInfo.valid;
            }

            break;
        }
    }
}

bool Shell::isDevSuitable(const PhysicalDeviceProperties &props, uint32_t &graphicsIndex, uint32_t &presentIndex,
                          uint32_t &transferIndex, uint32_t &computeIndex) {
    if (!determineQueueFamiliesSupport(props, graphicsIndex, presentIndex, transferIndex, computeIndex)) return false;
    if (!determineDeviceExtensionSupport(props)) return false;
    // optional (warn but dont throw)
    determineDeviceFeatureSupport(props);
    // depends on above...
    determineSampleCount(props);
    return true;
}

bool Shell::determineQueueFamiliesSupport(const PhysicalDeviceProperties &props, uint32_t &graphicsIndex,
                                          uint32_t &presentIndex, uint32_t &transferIndex, uint32_t &computeIndex) {
    // Determine graphics and present queues ...

    // Iterate over each queue to learn whether it supports presenting:
    std::vector<VkBool32> pSupportsPresent(props.queueFamilyCount);
    // TODO: I don't feel like re-arranging the whole start up atm.
    auto surface = createSurface(ctx_.instance);
    for (uint32_t i = 0; i < props.queueFamilyCount; i++) {
        vk::assert_success(vkGetPhysicalDeviceSurfaceSupportKHR(props.device, i, surface, pSupportsPresent.data()));

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that supports both
        if (props.queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (graphicsIndex == UINT32_MAX) graphicsIndex = i;

            if (pSupportsPresent[i] == VK_TRUE) {
                graphicsIndex = i;
                presentIndex = i;
                break;
            }
        }
    }
    if (presentIndex == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        // TODO: is this the best thing to do?
        for (uint32_t i = 0; i < props.queueFamilyCount; ++i) {
            if (pSupportsPresent[i] == VK_TRUE) {
                presentIndex = i;
                break;
            }
        }
    }
    vkDestroySurfaceKHR(ctx_.instance, surface, nullptr);

    // Determine transfer queue ...

    for (uint32_t i = 0; i < props.queueFamilyCount; ++i) {
        if ((props.queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
            props.queueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            transferIndex = i;
            break;
        } else if (props.queueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            transferIndex = i;
        }
    }

    // Determine compute queue ... (This logic ain't great. It can put compute with transfer)

    for (uint32_t i = 0; i < props.queueFamilyCount; ++i) {
        if ((props.queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
            props.queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            computeIndex = i;
            break;
        } else if (props.queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            computeIndex = i;
        }
    }

    // Do any other selection of device logic here...

    // Just pick the fist one that supports both
    return (graphicsIndex != UINT32_MAX && presentIndex != UINT32_MAX && transferIndex != UINT32_MAX &&
            computeIndex != UINT32_MAX);
}

bool Shell::determineDeviceExtensionSupport(const PhysicalDeviceProperties &props) {
    if (deviceExtensionInfo_.empty()) return true;
    for (const auto &extInfo : props.phyDevExtInfos) {
        if (extInfo.required && !extInfo.valid) return false;
    }
    return true;
}

void Shell::determineDeviceFeatureSupport(const PhysicalDeviceProperties &props) {
    // sampler anisotropy
    ctx_.samplerAnisotropyEnabled = props.features.samplerAnisotropy && settings_.try_sampler_anisotropy;
    if (settings_.try_sampler_anisotropy && !ctx_.samplerAnisotropyEnabled)
        log(LogPriority::LOG_WARN, "cannot enable sampler anisotropy");
    // sample rate shading
    ctx_.sampleRateShadingEnabled = props.features.sampleRateShading && settings_.try_sample_rate_shading;
    if (settings_.try_sample_rate_shading && !ctx_.sampleRateShadingEnabled)
        log(LogPriority::LOG_WARN, "cannot enable sample rate shading");
    // compute shading (TODO: this should be more robust)
    ctx_.computeShadingEnabled = props.features.fragmentStoresAndAtomics && settings_.try_compute_shading;
    if (settings_.try_compute_shading && !ctx_.computeShadingEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable compute shading (actually just can't enable fragment stores and atomics)");
    // tessellation shading
    ctx_.tessellationShadingEnabled = props.features.tessellationShader && settings_.try_tessellation_shading;
    if (settings_.try_tessellation_shading && !ctx_.tessellationShadingEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable tessellation shading");
    // geometry shading
    ctx_.geometryShadingEnabled = props.features.geometryShader && settings_.try_geometry_shading;
    if (settings_.try_geometry_shading && !ctx_.geometryShadingEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable geometry shading");
    // wireframe shading
    ctx_.wireframeShadingEnabled = props.features.fillModeNonSolid && settings_.try_wireframe_shading;
    if (settings_.try_wireframe_shading && !ctx_.wireframeShadingEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable wire frame shading (actually just can't enable fill mode non-solid)");
    // independent attachment blending
    ctx_.independentBlendEnabled = props.features.independentBlend && settings_.try_independent_blend;
    if (settings_.try_independent_blend && !ctx_.independentBlendEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable independent attachment blending");
}

void Shell::determineSampleCount(const PhysicalDeviceProperties &props) {
    /* DEPENDS on determine_device_feature_support */
    ctx_.samples = VK_SAMPLE_COUNT_1_BIT;
    if (ctx_.sampleRateShadingEnabled) {
        VkSampleCountFlags counts = std::min(props.properties.limits.framebufferColorSampleCounts,
                                             props.properties.limits.framebufferDepthSampleCounts);
        // return the highest possible one for now
        if (counts & VK_SAMPLE_COUNT_64_BIT)
            ctx_.samples = VK_SAMPLE_COUNT_64_BIT;
        else if (counts & VK_SAMPLE_COUNT_32_BIT)
            ctx_.samples = VK_SAMPLE_COUNT_32_BIT;
        else if (counts & VK_SAMPLE_COUNT_16_BIT)
            ctx_.samples = VK_SAMPLE_COUNT_16_BIT;
        else if (counts & VK_SAMPLE_COUNT_8_BIT)
            ctx_.samples = VK_SAMPLE_COUNT_8_BIT;
        else if (counts & VK_SAMPLE_COUNT_4_BIT)
            ctx_.samples = VK_SAMPLE_COUNT_4_BIT;
        else if (counts & VK_SAMPLE_COUNT_2_BIT)
            ctx_.samples = VK_SAMPLE_COUNT_2_BIT;
    }
}

bool Shell::determineSwapchainExtent(uint32_t widthHint, uint32_t heightHint, bool refreshCapabilities) {
    // 0, 0 for hints indicates calling from acquire_back_buffer... TODO: more robust solution???
    if (refreshCapabilities)
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx_.physicalDev, ctx_.surface, &ctx_.surfaceProps.capabilities);

    auto &caps = ctx_.surfaceProps.capabilities;

    VkExtent2D extent = caps.currentExtent;

    // use the hints
    if (extent.width == (uint32_t)-1) {
        extent.width = widthHint;
        extent.height = heightHint;
    }
    // clamp width; to protect us from broken hints?
    if (extent.width < caps.minImageExtent.width)
        extent.width = caps.minImageExtent.width;
    else if (extent.width > caps.maxImageExtent.width)
        extent.width = caps.maxImageExtent.width;
    // clamp height
    if (extent.height < caps.minImageExtent.height)
        extent.height = caps.minImageExtent.height;
    else if (extent.height > caps.maxImageExtent.height)
        extent.height = caps.maxImageExtent.height;

    if (ctx_.extent.width == extent.width && ctx_.extent.height == extent.height) {
        return true;
    } else {
        ctx_.extent = extent;
        return false;
    }
}

void Shell::determineDepthFormat() {
    /* allow custom depth formats */
#ifdef __ANDROID__
    // Depth format needs to be VK_FORMAT_D24_UNORM_S8_UINT on Android.
    ctx_.depth_format = VK_FORMAT_D24_UNORM_S8_UINT;
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    if (ctx_.depth_format == VK_FORMAT_UNDEFINED) ctx_.depth_format = VK_FORMAT_D32_SFLOAT;
#else
    if (ctx_.depthFormat == VK_FORMAT_UNDEFINED) ctx_.depthFormat = helpers::findDepthFormat(ctx_.physicalDev);
        // TODO: turn off depth if undefined still...
#endif
}

void Shell::determineSwapchainSurfaceFormat() {
    // no preferred type
    if (ctx_.surfaceProps.surfFormats.size() == 1 && ctx_.surfaceProps.surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        ctx_.surfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        return;
    } else {
        for (const auto &surfFormat : ctx_.surfaceProps.surfFormats) {
            if (surfFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                surfFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                ctx_.surfaceFormat = surfFormat;
                return;
            }
        }
    }
    log(LogPriority::LOG_INFO, "choosing first available swap surface format!");
    ctx_.surfaceFormat = ctx_.surfaceProps.surfFormats[0];
}

void Shell::determineSwapchainPresentMode() {
    // guaranteed to be present (vert sync)
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto &mode : ctx_.surfaceProps.presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {  // triple buffer
            presentMode = mode;
        } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {  // no sync
            presentMode = mode;
        }
    }
    ctx_.mode = presentMode;
}

void Shell::determineSwapchainImageCount() {
    auto &caps = ctx_.surfaceProps.capabilities;
    // Determine the number of VkImage's to use in the swap chain.
    // We need to acquire only 1 presentable image at at time.
    // Asking for minImageCount images ensures that we can acquire
    // 1 presentable image as long as we present it before attempting
    // to acquire another.
    ctx_.imageCount = settings_.back_buffer_count;
    if (ctx_.imageCount < caps.minImageCount)
        ctx_.imageCount = caps.minImageCount;
    else if (ctx_.imageCount > caps.maxImageCount)
        ctx_.imageCount = caps.maxImageCount;
}

void Shell::determineApiVersion(uint32_t &version) {
    version = VK_API_VERSION_1_0;
    // Android build not at 1.1 yet
#ifndef ANDROID
    // Keep track of the major/minor version we can actually use
    uint16_t using_major_version = 1;
    uint16_t using_minor_version = 0;
    std::string using_version_string = "";

    // Set the desired version we want
    uint32_t desired_version = getDesiredVersion();
    uint16_t desired_major_version = VK_VERSION_MAJOR(desired_version);
    uint16_t desired_minor_version = VK_VERSION_MINOR(desired_version);
    std::string desired_version_string = std::to_string(desired_major_version) + "." + std::to_string(desired_minor_version);

    VkInstance instance = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> physical_devices_desired;

    // Determine what API version is available
    uint32_t api_version;
    if (VK_SUCCESS == vkEnumerateInstanceVersion(&api_version)) {
        // Translate the version into major/minor for easier comparison
        uint32_t loader_major_version = VK_VERSION_MAJOR(api_version);
        uint32_t loader_minor_version = VK_VERSION_MINOR(api_version);
        std::cout << "Loader/Runtime support detected for Vulkan " << loader_major_version << "." << loader_minor_version
                  << "\n";

        // Check current version against what we want to run
        if (loader_major_version > desired_major_version ||
            (loader_major_version == desired_major_version && loader_minor_version >= desired_minor_version)) {
            // Initialize the VkApplicationInfo structure with the version of the API we're intending to use
            VkApplicationInfo app_info = {};
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pNext = nullptr;
            app_info.pApplicationName = "";
            app_info.applicationVersion = 1;
            app_info.pEngineName = "";
            app_info.engineVersion = 1;
            app_info.apiVersion = desired_version;

            // Initialize the VkInstanceCreateInfo structure
            VkInstanceCreateInfo inst_info = {};
            inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            inst_info.pNext = nullptr;
            inst_info.flags = 0;
            inst_info.pApplicationInfo = &app_info;
            inst_info.enabledExtensionCount = 0;
            inst_info.ppEnabledExtensionNames = nullptr;
            inst_info.enabledLayerCount = 0;
            inst_info.ppEnabledLayerNames = nullptr;

            // Attempt to create the instance
            if (VK_SUCCESS != vkCreateInstance(&inst_info, nullptr, &instance)) {
                std::cout << "Unknown error creating " << desired_version_string << " Instance\n";
                exit(-1);
            }

            // Get the list of physical devices
            uint32_t phys_dev_count = 1;
            if (VK_SUCCESS != vkEnumeratePhysicalDevices(instance, &phys_dev_count, nullptr) || phys_dev_count == 0) {
                std::cout << "Failed searching for Vulkan physical devices\n";
                exit(-1);
            }
            std::vector<VkPhysicalDevice> physical_devices;
            physical_devices.resize(phys_dev_count);
            if (VK_SUCCESS != vkEnumeratePhysicalDevices(instance, &phys_dev_count, physical_devices.data()) ||
                phys_dev_count == 0) {
                std::cout << "Failed enumerating Vulkan physical devices\n";
                exit(-1);
            }

            // Go through the list of physical devices and select only those that are capable of running the API version
            // we want.
            for (uint32_t dev = 0; dev < physical_devices.size(); ++dev) {
                VkPhysicalDeviceProperties physical_device_props = {};
                vkGetPhysicalDeviceProperties(physical_devices[dev], &physical_device_props);
                if (physical_device_props.apiVersion >= desired_version) {
                    physical_devices_desired.push_back(physical_devices[dev]);
                }
            }

            // If we have something in the desired version physical device list, we're good
            if (physical_devices_desired.size() > 0) {
                using_major_version = desired_major_version;
                using_minor_version = desired_minor_version;
            }
        }
    }

    using_version_string += std::to_string(using_major_version);
    using_version_string += ".";
    using_version_string += std::to_string(using_minor_version);

    if (using_minor_version != desired_minor_version) {
        std::cout << "Determined that this system can only use Vulkan API version " << using_version_string
                  << " instead of desired version " << desired_version_string << std::endl;
    } else {
        version = desired_version;
        std::cout << "Determined that this system can run desired Vulkan API version " << desired_version_string
                  << std::endl;
    }

    // Destroy the instance if it was created
    if (VK_NULL_HANDLE == instance) {
        vkDestroyInstance(instance, nullptr);
    }
#endif
}
