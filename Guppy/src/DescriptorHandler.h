/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef DESCRIPTOR_HANDLER_H
#define DESCRIPTOR_HANDLER_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vulkan/vulkan.hpp>

#include "Descriptor.h"
#include "ConstantsAll.h"
#include "Game.h"
#include "DescriptorSet.h"

// clang-format off
namespace Texture   { class Base; }
// clang-format on

namespace Descriptor {

// HANDLER

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;

    // POOL
    inline const vk::DescriptorPool& getPool() { return pool_; }

    // SET
    const Descriptor::Set::Base& getDescriptorSet(const DESCRIPTOR_SET& type) const {
        for (auto& pSet : pDescriptorSets_) {
            if (pSet->TYPE == type) return std::ref(*pSet.get());
        }
        assert(false);
        throw std::runtime_error("Unrecognized set type");
    }
    Descriptor::Set::resourceHelpers getResourceHelpers(
        const std::set<PASS> passTypes, const PIPELINE& pipelineType,
        const Descriptor::Set::typeShaderStagePairs& descSetStagePairs) const;

    // DESCRIPTOR
    void getBindData(const PIPELINE& pipelineType, Descriptor::Set::bindDataMap& bindDataMap,
                     const std::vector<Descriptor::Base*> pDynamicItems = {});
    void updateBindData(const std::vector<std::string> textureIds);

   private:
    void reset() override;

    // POOL
    void createPool();
    vk::DescriptorPool pool_;

    // LAYOUT
    void createLayouts();

    // DESCRIPTOR
    uint32_t getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets) const;

    inline std::unique_ptr<Descriptor::Set::Base>& getSet(const DESCRIPTOR_SET& type) {
        for (auto& pSet : pDescriptorSets_) {
            if (pSet->TYPE == type) return pSet;
        }
        assert(false);
        exit(EXIT_FAILURE);
    }

    void prepareDescriptorSet(std::unique_ptr<Descriptor::Set::Base>& pSet);

    void allocateDescriptorSets(const Descriptor::Set::Resource& resource, std::vector<vk::DescriptorSet>& descriptorSets);

    void updateDescriptorSets(const Descriptor::bindingMap& bindingMap, const Descriptor::OffsetsMap& offsets,
                              Set::resourceInfoMapSetsPair& pair, const std::vector<Descriptor::Base*> pDynamicItems) const;

    vk::WriteDescriptorSet getWrite(const Descriptor::bindingMapKeyValue& keyValue, const vk::DescriptorSet& set) const;
    void getDynamicOffsets(const std::unique_ptr<Descriptor::Set::Base>& pSet, std::vector<uint32_t>& dynamicOffsets,
                           const std::vector<Descriptor::Base*> pDynamicItems);

    // BINDING
    vk::DescriptorSetLayoutBinding getDecriptorSetLayoutBinding(const Descriptor::bindingMapKeyValue& keyValue,
                                                                const vk::ShaderStageFlags& stageFlags,
                                                                const Uniform::offsets& offsets) const;

    std::vector<std::unique_ptr<Descriptor::Set::Base>> pDescriptorSets_;
};

}  // namespace Descriptor

#endif  // !DESCRIPTOR_HANDLER_H
