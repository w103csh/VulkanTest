/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Ocean.h"

#include <cmath>
#include <complex>
#include <random>
#include <string>
#include <vulkan/vulkan.h>

#include "Deferred.h"
#include "FFT.h"
#include "Helpers.h"
#include "Tessellation.h"
// HANDLERS
#include "LoadingHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "TextureHandler.h"

namespace {

float phillipsSpectrum(const glm::vec2 k, const float kMagnitude, const Ocean::SurfaceCreateInfo& info) {
    if (kMagnitude < 1e-5f) return 0.0f;
    float kHatOmegaHat = glm::dot(k, info.omega) / kMagnitude;  // cosine factor
    float kMagnitude2 = kMagnitude * kMagnitude;
    float damp = exp(-kMagnitude2 * info.l * info.l);
    float phk = info.A * damp * (exp(-1.0f / (kMagnitude2 * info.L * info.L)) / (kMagnitude2 * kMagnitude2)) *
                (kHatOmegaHat * kHatOmegaHat);
    return phk;
}

}  // namespace

// TEXUTRE
namespace Texture {
namespace Ocean {

void MakTextures(Handler& handler, const ::Ocean::SurfaceCreateInfo& info) {
    assert(helpers::isPowerOfTwo(info.N) && helpers::isPowerOfTwo(info.M));

    std::default_random_engine gen;
    std::normal_distribution<float> dist(0.0, 1.0);

    auto dataSize = (static_cast<uint64_t>(info.N) * 4) * static_cast<uint64_t>(info.M) * sizeof(float);

    // Wave vector data
    float* pWave = (float*)malloc(dataSize);
    assert(pWave);

    // Fourier domain amplitude data
    float* pHTilde0 = (float*)malloc(dataSize);
    assert(pHTilde0);

    const int halfN = info.N / 2, halfM = info.M / 2;
    int idx;
    float kx, kz, kMagnitude, phk;
    std::complex<float> hTilde0, hTilde0Conj, xhi;

    for (int i = 0, m = -halfM; i < static_cast<int>(info.M); i++, m++) {
        for (int j = 0, n = -halfN; j < static_cast<int>(info.N); j++, n++) {
            idx = (i * info.N * 4) + (j * 4);

            // Wave vector data
            {
                kx = 2.0f * glm::pi<float>() * n / info.Lx;
                kz = 2.0f * glm::pi<float>() * m / info.Lz;
                kMagnitude = sqrt(kx * kx + kz * kz);

                pWave[idx + 0] = kx;
                pWave[idx + 1] = kz;
                pWave[idx + 2] = kMagnitude;
                pWave[idx + 3] = sqrt(::Ocean::g * kMagnitude);
            }

            // Fourier domain data
            {
                phk = phillipsSpectrum({kx, kz}, kMagnitude, info);
                xhi = {dist(gen), dist(gen)};
                hTilde0 = xhi * sqrt(phk / 2.0f);
                // conjugate
                phk = phillipsSpectrum({-kx, -kz}, kMagnitude, info);
                xhi = {dist(gen), dist(gen)};
                hTilde0Conj = std::conj(xhi * sqrt(phk / 2.0f));

                pHTilde0[idx + 0] = hTilde0.real();
                pHTilde0[idx + 1] = hTilde0.imag();
                pHTilde0[idx + 2] = hTilde0Conj.real();
                pHTilde0[idx + 3] = hTilde0Conj.imag();
            }
        }
    }

    // Create texture
    {
        Sampler::CreateInfo sampInfo = {
            std::string(DATA_ID) + " Sampler",
            {{
                 {::Sampler::USAGE::DONT_CARE},  // wave vector
                 {::Sampler::USAGE::DONT_CARE},  // fourier domain
                 {::Sampler::USAGE::HEIGHT},     // fourier domain dispersion relation (height)
                 {::Sampler::USAGE::NORMAL},     // fourier domain dispersion relation (slope)
                 {::Sampler::USAGE::DONT_CARE},  // fourier domain dispersion relation (differential)
             },
             true,
             true},
            VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            {info.N, info.M, 1},
            {},
            0,
            SAMPLER::DEFAULT_NEAREST,  // Maybe this texture should be split up for filtering layers separately?
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            {{false, false}, 1},
            VK_FORMAT_R32G32B32A32_SFLOAT,
            Sampler::CHANNELS::_4,
            sizeof(float),
        };

        for (size_t i = 0; i < sampInfo.layersInfo.infos.size(); i++) {
            if (i == 0)
                sampInfo.layersInfo.infos[i].pPixel = pWave;
            else if (i == 1)
                sampInfo.layersInfo.infos[i].pPixel = pHTilde0;
            else
                sampInfo.layersInfo.infos[i].pPixel = (float*)malloc(dataSize);  // TODO: this shouldn't be necessary
        }

        Texture::CreateInfo waveTexInfo = {std::string(DATA_ID), {sampInfo}, false, false, STORAGE_IMAGE::DONT_CARE};
        handler.make(&waveTexInfo);
    }
}

}  // namespace Ocean
}  // namespace Texture

// SHADER
namespace Shader {
namespace Ocean {
const CreateInfo DISP_COMP_CREATE_INFO = {
    SHADER::OCEAN_DISP_COMP,  //
    "Ocean Sufrace Dispersion Compute Shader",
    "comp.ocean.dispersion.glsl",
    VK_SHADER_STAGE_COMPUTE_BIT,
};
const CreateInfo FFT_COMP_CREATE_INFO = {
    SHADER::OCEAN_FFT_COMP,  //
    "Ocean Surface Fast Fourier Transform Compute Shader",
    "comp.ocean.fft.glsl",
    VK_SHADER_STAGE_COMPUTE_BIT,
};
const CreateInfo VERT_CREATE_INFO = {
    SHADER::OCEAN_VERT,  //
    "Ocean Surface Vertex Shader",
    "vert.ocean.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};
const CreateInfo DEFERRED_MRT_FRAG_CREATE_INFO = {
    SHADER::OCEAN_DEFERRED_MRT_FRAG,                                  //
    "Ocean Surface Deferred Multiple Render Target Fragment Shader",  //
    "frag.ocean.deferred.mrt.glsl",                                   //
    VK_SHADER_STAGE_FRAGMENT_BIT,                                     //
    {SHADER_LINK::COLOR_FRAG, SHADER_LINK::DEFAULT_MATERIAL},
};
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {
namespace Simulation {

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::OCEAN),
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    assert(helpers::isPowerOfTwo(pCreateInfo->info.N) && helpers::isPowerOfTwo(pCreateInfo->info.M));
    data_.nLog2 = static_cast<uint32_t>(log2(pCreateInfo->info.N));
    data_.mLog2 = static_cast<uint32_t>(log2(pCreateInfo->info.M));
    data_.lambda = pCreateInfo->info.lambda;
    data_.t = 0.0f;
    setData();
}

void Base::updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {
    data_.t = time;
    setData(frameIndex);
}

}  // namespace Simulation
}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo OCEAN_DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::OCEAN_DEFAULT,
    "_DS_OCEAN",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
        {{2, 0}, {UNIFORM_DYNAMIC::OCEAN}},
        {{3, 0}, {STORAGE_IMAGE::PIPELINE, Texture::Ocean::DATA_ID}},
        {{4, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_N_ID}},
        {{5, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_M_ID}},
        {{6, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_TWIDDLE_FACTORS_ID}},
    },
};
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {

namespace Ocean {

// DISPERSION (COMPUTE)
const CreateInfo DISP_CREATE_INFO = {
    COMPUTE::OCEAN_DISP,
    "Ocean Surface Dispersion Compute Pipeline",
    {SHADER::OCEAN_DISP_COMP},
    {DESCRIPTOR_SET::OCEAN_DEFAULT},
    {},
    {},
    {::Ocean::DISP_LOCAL_SIZE, ::Ocean::DISP_LOCAL_SIZE, 1},
};

Dispersion::Dispersion(Handler& handler)
    : Compute(handler, &DISP_CREATE_INFO), omega0_(2.0f * glm::pi<float>() / ::Ocean::T) {}

void Dispersion::getShaderStageInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.specializationMapEntries.push_back({{}});

    // Use specialization constants to pass number of samples to the shader (used for MSAA resolve)
    createInfoRes.specializationMapEntries.back().back().constantID = 0;
    createInfoRes.specializationMapEntries.back().back().offset = 0;
    createInfoRes.specializationMapEntries.back().back().size = sizeof(omega0_);

    createInfoRes.specializationInfo.push_back({});
    createInfoRes.specializationInfo.back().mapEntryCount =
        static_cast<uint32_t>(createInfoRes.specializationMapEntries.back().size());
    createInfoRes.specializationInfo.back().pMapEntries = createInfoRes.specializationMapEntries.back().data();
    createInfoRes.specializationInfo.back().dataSize = sizeof(omega0_);
    createInfoRes.specializationInfo.back().pData = &omega0_;

    assert(createInfoRes.shaderStageInfos.size() == 1 &&
           createInfoRes.shaderStageInfos[0].stage == VK_SHADER_STAGE_COMPUTE_BIT);
    // Add the specialization to the shader info.
    createInfoRes.shaderStageInfos[0].pSpecializationInfo = &createInfoRes.specializationInfo.back();
}

// FFT (COMPUTE)
const CreateInfo FFT_CREATE_INFO = {
    COMPUTE::OCEAN_FFT,
    "Ocean Surface FFT Compute Pipeline",
    {SHADER::OCEAN_FFT_COMP},
    {DESCRIPTOR_SET::OCEAN_DEFAULT},
    {},
    {PUSH_CONSTANT::FFT_ROW_COL_OFFSET},
    {::Ocean::FFT_LOCAL_SIZE, 1, 1},
};
FFT::FFT(Handler& handler) : Compute(handler, &FFT_CREATE_INFO) {}

// WIREFRAME
const CreateInfo OCEAN_WF_CREATE_INFO = {
    GRAPHICS::OCEAN_WF_DEFERRED,
    "Ocean Surface Wireframe (Deferred) Pipeline",
    {SHADER::OCEAN_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {DESCRIPTOR_SET::OCEAN_DEFAULT},
};
Wireframe::Wireframe(Handler& handler) : Graphics(handler, &OCEAN_WF_CREATE_INFO), DO_BLEND(false), IS_DEFERRED(true) {}

void Wireframe::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Wireframe::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    ::HeightFieldFluid::VertexData::getInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);
    Storage::Vector4::GetInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);  // Not used
    Instance::Obj3d::DATA::getInputDescriptions(createInfoRes);

    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_TRUE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
}

// SURFACE
const CreateInfo OCEAN_SURFACE_CREATE_INFO = {
    GRAPHICS::OCEAN_SURFACE_DEFERRED,
    "Ocean Surface (Deferred) Pipeline",
    {
        SHADER::OCEAN_VERT,
        SHADER::PHONG_TRI_COLOR_TESC,
        SHADER::PHONG_TRI_COLOR_TESE,
        SHADER::OCEAN_DEFERRED_MRT_FRAG,
    },
    {DESCRIPTOR_SET::OCEAN_DEFAULT, DESCRIPTOR_SET::TESS_PHONG},
};
Surface::Surface(Handler& handler)
    : Graphics(handler, &OCEAN_SURFACE_CREATE_INFO), DO_BLEND(false), DO_TESSELLATE(true), IS_DEFERRED(true) {}

void Surface::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Surface::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    ::HeightFieldFluid::VertexData::getInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);
    Storage::Vector4::GetInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);  // Not used
    Instance::Obj3d::DATA::getInputDescriptions(createInfoRes);

    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    if (DO_TESSELLATE) {
        createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
        createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    } else {
        createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_TRUE;
        createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
}

void Surface::getTesselationInfoResources(CreateInfoResources& createInfoRes) {
    if (!DO_TESSELLATE) return;
    createInfoRes.tessellationStateInfo = {};
    createInfoRes.tessellationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    createInfoRes.tessellationStateInfo.pNext = nullptr;
    createInfoRes.tessellationStateInfo.flags = 0;
    createInfoRes.tessellationStateInfo.patchControlPoints = 3;
}

}  // namespace Ocean

}  // namespace Pipeline

// BUFFER
namespace Ocean {

Buffer::Buffer(Particle::Handler& handler, const Particle::Buffer::index&& offset, const CreateInfo* pCreateInfo,
               std::shared_ptr<Material::Base>& pMaterial,
               const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
               std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData)
    : Buffer::Base(handler, std::forward<const Particle::Buffer::index>(offset), pCreateInfo, pMaterial, pDescriptors),
      Obj3d::InstanceDraw(pInstanceData),
      normalOffset_(Particle::Buffer::BAD_OFFSET),
      indexWFRes_{},
      drawMode(GRAPHICS::OCEAN_SURFACE_DEFERRED),
      info_(pCreateInfo->info) {
    assert(info_.N == info_.M);  // Needs to be square currently.
    assert(info_.N == FFT_LOCAL_SIZE * FFT_WORKGROUP_SIZE);
    assert(info_.N == DISP_LOCAL_SIZE * DISP_WORKGROUP_SIZE);
    assert(helpers::isPowerOfTwo(N) && helpers::isPowerOfTwo(M));

    const auto& N = info_.N;
    const auto& M = info_.M;

    for (uint32_t i = 0; i < static_cast<uint32_t>(pDescriptors_.size()); i++)
        if (pDescriptors[i]->getDescriptorType() == DESCRIPTOR{STORAGE_BUFFER_DYNAMIC::NORMAL}) normalOffset_ = i;
    assert(normalOffset_ != Particle::Buffer::BAD_OFFSET);

    // IMAGE
    // TODO: move the loop inside this function into the constructor here.
    Texture::Ocean::MakTextures(handler.textureHandler(), info_);

    // BUFFER VIEWS
    {
        auto bitRevOffsets = FFT::MakeBitReversalOffsets(N);
        handler.textureHandler().makeBufferView(BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_N_ID, VK_FORMAT_R16_SINT,
                                                sizeof(uint16_t) * bitRevOffsets.size(), bitRevOffsets.data());
        if (N != M) bitRevOffsets = FFT::MakeBitReversalOffsets(N);
        handler.textureHandler().makeBufferView(BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_M_ID, VK_FORMAT_R16_SINT,
                                                sizeof(uint16_t) * bitRevOffsets.size(), bitRevOffsets.data());
        auto twiddleFactors = FFT::MakeTwiddleFactors((std::max)(N, M));
        handler.textureHandler().makeBufferView(BufferView::Ocean::FFT_TWIDDLE_FACTORS_ID, VK_FORMAT_R32G32_SFLOAT,
                                                sizeof(float) * twiddleFactors.size(), twiddleFactors.data());
    }

    const int halfN = N / 2, halfM = M / 2;
    verticesHFF_.reserve(N * M);
    verticesHFF_.reserve(N * M);
    for (int i = 0, m = -halfM; i < static_cast<int>(M); i++, m++) {
        for (int j = 0, n = -halfN; j < static_cast<int>(N); j++, n++) {
            // VERTEX
            verticesHFF_.push_back({
                // position
                {
                    n * info_.Lx / N,
                    0.0f,
                    m * info_.Lz / M,
                },
                // normal
                // image offset
                {static_cast<int>(j), static_cast<int>(i)},
            });
        }
    }

    // INDEX
    {
        bool doPatchList = true;
        if (!doPatchList) {
            size_t numIndices = static_cast<size_t>(static_cast<size_t>(N) * (2 + ((static_cast<size_t>(M) - 2) * 2)) +
                                                    static_cast<size_t>(M) - 1);
            indices_.resize(numIndices);
        } else {
            indices_.reserve((N - 1) * (M - 1) * 6);
        }

        size_t index = 0;
        for (uint32_t row = 0; row < M - 1; row++) {
            auto rowOffset = row * N;
            for (uint32_t col = 0; col < N; col++) {
                if (!doPatchList) {
                    // Triangle strip indices (surface)
                    indices_[index + 1] = (row + 1) * N + col;
                    indices_[index] = row * N + col;
                    index += 2;
                } else {
                    // Patch list (surface)
                    if (col < N - 1) {
                        indices_.push_back(col + rowOffset);
                        indices_.push_back(col + rowOffset + N);
                        indices_.push_back(col + rowOffset + 1);
                        indices_.push_back(col + rowOffset + 1);
                        indices_.push_back(col + rowOffset + N);
                        indices_.push_back(col + rowOffset + N + 1);
                    }
                }
                // Line strip indices (wireframe)
                indicesWF_.push_back(row * N + col + N);
                indicesWF_.push_back(indicesWF_[indicesWF_.size() - 1] - N);
                if (col + 1 < N) {
                    indicesWF_.push_back(indicesWF_[indicesWF_.size() - 1] + 1);
                    indicesWF_.push_back(indicesWF_[indicesWF_.size() - 3]);
                }
            }
            if (!doPatchList) {
                indices_[index++] = VB_INDEX_PRIMITIVE_RESTART;
            }
            indicesWF_.push_back(VB_INDEX_PRIMITIVE_RESTART);
        }
    }

    status_ |= STATUS::PENDING_BUFFERS;
    draw_ = true;
    paused_ = false;
}

void Buffer::loadBuffers() {
    const auto& ctx = handler().shell().context();
    pLdgRes_ = handler().loadingHandler().createLoadingResources();

    // VERTEX
    BufferResource stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
        sizeof(HeightFieldFluid::VertexData) * verticesHFF_.size(), NAME + " vertex", stgRes, verticesHFFRes_,
        verticesHFF_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // INDEX (SURFACE)
    assert(indices_.size());
    stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
        sizeof(VB_INDEX_TYPE) * indices_.size(), NAME + " index (surface)", stgRes, indexRes_, indices_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // INDEX (WIREFRAME)
    assert(indicesWF_.size());
    stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
        sizeof(VB_INDEX_TYPE) * indicesWF_.size(), NAME + " index (wireframe)", stgRes, indexWFRes_, indicesWF_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));
}

void Buffer::destroy() {
    Base::destroy();
    auto& dev = handler().shell().context().dev;
    if (verticesHFF_.size()) {
        vkDestroyBuffer(dev, verticesHFFRes_.buffer, nullptr);
        vkFreeMemory(dev, verticesHFFRes_.memory, nullptr);
    }
    if (indicesWF_.size()) {
        vkDestroyBuffer(dev, indexWFRes_.buffer, nullptr);
        vkFreeMemory(dev, indexWFRes_.memory, nullptr);
    }
}

void Buffer::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                  const uint8_t frameIndex) const {
    if (pPipelineBindData->type != PIPELINE{drawMode}) return;

    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    switch (drawMode) {
        case GRAPHICS::OCEAN_WF_DEFERRED: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());
            const VkBuffer buffers[] = {
                verticesHFFRes_.buffer,
                pDescriptors_[normalOffset_]->BUFFER_INFO.bufferInfo.buffer,  // Not used
                pInstObj3d_->BUFFER_INFO.bufferInfo.buffer,

            };
            const VkDeviceSize offsets[] = {
                0,
                pDescriptors_[normalOffset_]->BUFFER_INFO.memoryOffset,  // Not used
                pInstObj3d_->BUFFER_INFO.memoryOffset,
            };
            vkCmdBindVertexBuffers(cmd, 0, 3, buffers, offsets);
            vkCmdBindIndexBuffer(cmd, indexWFRes_.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(                              //
                cmd,                                       // VkCommandBuffer commandBuffer
                static_cast<uint32_t>(indicesWF_.size()),  // uint32_t indexCount
                pInstObj3d_->BUFFER_INFO.count,            // uint32_t instanceCount
                0,                                         // uint32_t firstIndex
                0,                                         // int32_t vertexOffset
                0                                          // uint32_t firstInstance
            );
        } break;
        case GRAPHICS::OCEAN_SURFACE_DEFERRED: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());
            const VkBuffer buffers[] = {
                verticesHFFRes_.buffer,
                pDescriptors_[normalOffset_]->BUFFER_INFO.bufferInfo.buffer,
                pInstObj3d_->BUFFER_INFO.bufferInfo.buffer,
            };
            const VkDeviceSize offsets[] = {
                0,
                pDescriptors_[normalOffset_]->BUFFER_INFO.memoryOffset,
                pInstObj3d_->BUFFER_INFO.memoryOffset,
            };
            vkCmdBindVertexBuffers(cmd, 0, 3, buffers, offsets);
            vkCmdBindIndexBuffer(cmd, indexRes_.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(                            //
                cmd,                                     // VkCommandBuffer commandBuffer
                static_cast<uint32_t>(indices_.size()),  // uint32_t indexCount
                pInstObj3d_->BUFFER_INFO.count,          // uint32_t instanceCount
                0,                                       // uint32_t firstIndex
                0,                                       // int32_t vertexOffset
                0                                        // uint32_t firstInstance
            );
        } break;
        default: {
            assert(false);
        } break;
    }
}

void Buffer::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                      const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    const VkMemoryBarrier memoryBarrierCompute = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,  // srcAccessMask
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,  // dstAccessMask
    };

    switch (std::visit(Pipeline::GetCompute{}, pPipelineBindData->type)) {
        case COMPUTE::FFT_ONE: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());

            vkCmdDispatch(cmd, 1, 4, 1);

            vkCmdPipelineBarrier(cmd,                                   //
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
                                 0,                                     // dependencyFlags
                                 1,                                     // memoryBarrierCount
                                 &memoryBarrierCompute,                 // pMemoryBarriers
                                 0, nullptr, 0, nullptr);

            vkCmdDispatch(cmd, 4, 1, 1);

        } break;
        case COMPUTE::OCEAN_DISP: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());

            vkCmdDispatch(cmd, DISP_WORKGROUP_SIZE, DISP_WORKGROUP_SIZE, 1);

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
                                 &memoryBarrierCompute, 0, nullptr, 0, nullptr);

        } break;
        case COMPUTE::OCEAN_FFT: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());

            FFT::RowColumnOffset offset = 1;  // row
            vkCmdPushConstants(cmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                               static_cast<uint32_t>(sizeof(FFT::RowColumnOffset)), &offset);

            vkCmdDispatch(cmd, FFT_WORKGROUP_SIZE, 1, 1);

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
                                 &memoryBarrierCompute, 0, nullptr, 0, nullptr);

            offset = 0;  // column
            vkCmdPushConstants(cmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                               static_cast<uint32_t>(sizeof(FFT::RowColumnOffset)), &offset);

            vkCmdDispatch(cmd, FFT_WORKGROUP_SIZE, 1, 1);

        } break;
        default: {
            assert(false);
        } break;
    }
}

}  // namespace Ocean