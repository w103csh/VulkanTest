//#ifndef SAMPLER_H
//#define SAMPLER_H
//
//#include <string>
//#include <vector>
//#include <vulkan/vulkan.h>
//
//#include "Game.h"
//#include "Helpers.h"
//
// namespace Sampler {
//
// class Base {
//   public:
//    virtual VkDescriptorSetLayoutBinding getDecriptorLayoutBinding(uint32_t binding, uint32_t count) const = 0;
//
//    virtual void createDescriptorSets(const VkDevice &dev, const VkDescriptorPool &pool, const VkDescriptorBufferInfo
//    &info,
//                                      std::vector<VkDescriptorSet> &sets, uint32_t imageCount, uint32_t count) = 0;
//
//    void destroy(const VkDevice &dev);
//
//    DESCRIPTOR_TYPE TYPE;
//
//   protected:
//    Base(DESCRIPTOR_TYPE &&type, std::string &&name) : TYPE(type), resources_(), name_(name){};
//
//    void init(const VkDevice &dev, const Game::Settings &settings, VkDeviceSize &size, uint32_t count = 1);
//
//    inline void mapResources(const VkDevice &dev, uint8_t *&pData) {
//        vk::assert_success(vkMapMemory(dev, resources_.memory, 0, resources_.size, 0, (void **)&pData));
//    }
//
//    inline void unmapResources(const VkDevice &dev) { vkUnmapMemory(dev, resources_.memory); }
//
//   private:
//    std::string name_;
//    DescriptorResources resources_;
//};
//
//// **********************
////      Default
//// **********************
//
// class Default : public Base {
//   public:
//    Default::Default() : Base(DESCRIPTOR_TYPE::DEFAULT_UNIFORM, "Default"){};
//
//    typedef enum FLAGS {
//        DEFAULT = 0x00000000,
//        // Should this be on the "Dynamic" uniform buffer? Now its on "Default". If it stays then
//        // move it to Scene.
//        TOON_SHADE = 0x00000001,
//        // THROUGH 0x0000000F
//        FOG_LINEAR = 0x00000010,
//        FOG_EXP = 0x00000020,
//        FOG_EXP2 = 0x00000040,
//        // THROUGH 0x0000000F (used by shader)
//        BITS_MAX_ENUM = 0x7FFFFFFF
//    } FLAGS;
//    struct DATA {
//        const Camera::Data *pCamera = nullptr;
//        struct ShaderData {
//            alignas(16) FlagBits flags = FLAGS::DEFAULT;
//            // 12 rem
//            struct Fog {
//                float minDistance = 0.0f;
//                float maxDistance = 40.0f;
//                float density = 0.05f;
//                alignas(4) float __padding1;                // rem 4
//                alignas(16) glm::vec3 color = CLEAR_COLOR;  // rem 4
//            } fog;
//        } shaderData;
//        std::vector<Light::PositionalData> positionalLightData;
//        std::vector<Light::SpotData> spotLightData;
//    } data;
//
//    const uint32_t ARRAY_ELEMENT = 0;  // Force any shaders that use this uniform to use array element 0. For now...
//
//    void init(const VkDevice &dev, const Game::Settings &settings, const Camera &camera);
//    void update(const VkDevice &dev, Camera &camera);
//
//    VkDescriptorSetLayoutBinding getDecriptorLayoutBinding(
//        uint32_t binding = static_cast<uint32_t>(DESCRIPTOR_TYPE::DEFAULT_UNIFORM), uint32_t count = 1) const override;
//
//    void createDescriptorSets(const VkDevice &dev, const VkDescriptorPool &pool, const VkDescriptorBufferInfo &info,
//                              std::vector<VkDescriptorSet> &sets, uint32_t imageCount, uint32_t count) override;
//
//    void uniformAction(std::function<void(DATA &)> &&updateFunc) { updateFunc(data); }
//
//    // LIGHTS
//    uint16_t &&getPositionLightCount() { return std::move(static_cast<uint16_t>(positionalLights_.size())); }
//    uint16_t &&getSpotLightCount() { return std::move(static_cast<uint16_t>(spotLights_.size())); }
//    void lightsAction(std::function<void(std::vector<Light::Positional> &, std::vector<Light::Spot> &)> &&updateFunc) {
//        updateFunc(positionalLights_, spotLights_);
//    }
//
//   private:
//    void copy(const VkDevice &dev);
//
//    // LIGHTS
//    std::vector<Light::Positional> positionalLights_;
//    std::vector<Light::Spot> spotLights_;
//};
//
//#endif  // !SAMPLER_H