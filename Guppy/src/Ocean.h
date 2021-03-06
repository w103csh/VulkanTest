/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OCEAN_H
#define OCEAN_H

#include <glm/glm.hpp>
#include <memory>
#include <string_view>
#include <vulkan/vulkan.hpp>

#include <CDLOD/VkGridMesh.h>

#include "ConstantsAll.h"
#include "DescriptorManager.h"
#include "GraphicsWork.h"
#include "Instance.h"
#include "Pipeline.h"

// clang-format off
namespace Ocean   { class Renderer; }
namespace Pass    { class Handler; }
namespace Texture { class Handler; }
// clang-format on

namespace Ocean {

/**
 * Unfortunately the ocean data sample dimensions need to be square because of how the fft compute shader works currently.
 * The sample dimensions also need to be known here because of the pipeline creation order, and having to use a text
 * replacement routine for the local size value. Still not sure if specialization info could be used for something like that.
 * It didn't work the first time I tried it.
 */
constexpr uint32_t N = 256;
constexpr uint32_t M = N;
constexpr uint32_t FFT_LOCAL_SIZE = 64;
constexpr uint32_t FFT_WORKGROUP_SIZE = N / FFT_LOCAL_SIZE;
constexpr uint32_t DISP_LOCAL_SIZE = 32;
constexpr uint32_t DISP_WORKGROUP_SIZE = N / DISP_LOCAL_SIZE;

constexpr float T = 200.0f;  // wave repeat time
constexpr float g = 9.81f;   // gravity

struct SurfaceCreateInfo {
    SurfaceCreateInfo()
        : Lx(1000.0f),  //
          Lz(1000.0f),
          N(::Ocean::N),
          M(::Ocean::M),
          V(31.0f),
          omega(1.0f, 0.0f),
          l(1.0f),
          A(2e-5f),
          L(),
          lambda(-1.0f) {
        L = (V * V) / g;
    }
    float Lx;         // grid size (meters)
    float Lz;         // grid size (meters)
    uint32_t N;       // grid size (discrete Lx)
    uint32_t M;       // grid size (discrete Lz)
    float V;          // wind speed (meters/second)
    glm::vec2 omega;  // wind direction (normalized)
    float l;          // small wave cutoff (meters)
    float A;          // Phillips spectrum constant (wave amplitude?)
    float L;          // largest possible waves from continuous wind speed V
    float lambda;     // horizontal displacement scale factor
};

}  // namespace Ocean

// BUFFER VIEW
namespace BufferView {
namespace Ocean {
constexpr std::string_view FFT_BIT_REVERSAL_OFFSETS_N_ID = "Ocean FFT BRO N";
constexpr std::string_view FFT_BIT_REVERSAL_OFFSETS_M_ID = "Ocean FFT BRO M";
constexpr std::string_view FFT_TWIDDLE_FACTORS_ID = "Ocean FFT TF";
void MakeResources(Texture::Handler& handler, const ::Ocean::SurfaceCreateInfo& info);
}  // namespace Ocean
}  // namespace BufferView

// TEXTURE
namespace Texture {
class Handler;
struct CreateInfo;
namespace Ocean {

constexpr std::string_view WAVE_FOURIER_ID = "Ocean Wave & Fourier Data Texture";
constexpr std::string_view DISP_REL_ID = "Ocean Dispersion Relation Data Texture";
constexpr std::string_view VERT_INPUT_ID = "Ocean Vertex Shader Input Texture";
void MakeResources(Texture::Handler& handler, const ::Ocean::SurfaceCreateInfo& info);

constexpr std::string_view VERT_INPUT_COPY_ID = "Ocean Vertex Shader Input Copy Texture";
CreateInfo MakeCopyTexInfo(const uint32_t N, const uint32_t M);

}  // namespace Ocean
}  // namespace Texture

// SHADER
namespace Shader {
namespace Ocean {
extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo DEFERRED_MRT_FRAG_CREATE_INFO;
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {
namespace SimulationDraw {
struct DATA {
    bool NOTHING;  // This whole thing is unused atm.
};
struct CreateInfo : Buffer::CreateInfo {
    ::Ocean::SurfaceCreateInfo info;
};
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
};
using Manager = Descriptor::Manager<Descriptor::Base, Base, std::shared_ptr>;
}  // namespace SimulationDraw
}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo OCEAN_DRAW_CREATE_INFO;
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
class Handler;
namespace Ocean {

// WIREFRAME
class Wireframe : public Graphics {
   public:
    const bool DO_BLEND = false;
    const bool IS_DEFERRED = true;

    Wireframe(Handler& handler);

   protected:
    Wireframe(Handler& handler, const CreateInfo* pCreateInfo);

    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

// WIREFRAME (TESSELLLATION)
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
class WireframeTess : public Wireframe {
   public:
    WireframeTess(Handler& handler);

   private:
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
    void getTessellationInfoResources(CreateInfoResources& createInfoRes) override;
};
#endif

// SURFACE
class Surface : public Graphics {
   public:
    const bool DO_BLEND = false;
    const bool IS_DEFERRED = true;

    Surface(Handler& handler);

   protected:
    Surface(Handler& handler, const CreateInfo* pCreateInfo);

    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

// SURFACE (TESSELLATION)
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
class SurfaceTess : public Surface {
   public:
    SurfaceTess(Handler& handler);

   private:
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
    void getTessellationInfoResources(CreateInfoResources& createInfoRes) override;
};
#endif

}  // namespace Ocean
}  // namespace Pipeline

// INSTANCE
namespace Instance {
namespace Cdlod {
namespace Ocean {
struct DATA {
    static void getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes);
    glm::vec4 data0;  // quadOffset: .x.y
                      // quadScale:  .z.w
    glm::vec4 data1;  // uvOffset:   .x.y
                      // uvScale:    .z.w
};
class Base;
struct CreateInfo : public Instance::CreateInfo<DATA, Base> {};
class Base : public Buffer::DataItem<DATA>, public Instance::Base {
   public:
    Base(const Buffer::Info&& info, DATA* pData);
};
}  // namespace Ocean
}  // namespace Cdlod
}  // namespace Instance

// GRAPHICS WORK
namespace GraphicsWork {

struct OceanCreateInfo : CreateInfo {
    OceanCreateInfo() : CreateInfo() {}
    Ocean::SurfaceCreateInfo surfaceInfo;
};

class OceanSurface : public Base {
   public:
    OceanSurface(Pass::Handler& handler, const OceanCreateInfo* pCreateInfo, Ocean::Renderer* pRenderer);

    void init();
    void destroy();

    void record(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                const vk::CommandBuffer& cmd) override;

   private:
    void load(std::unique_ptr<LoadingResource>& pLdgRes) override;

    Ocean::Renderer* pRenderer_;
    Ocean::SurfaceCreateInfo surfaceInfo_;
    // DEBUG
    uint32_t gridMeshDims_;
    VkGridMesh gridMesh_;
    std::shared_ptr<Instance::Cdlod::Ocean::Base> pInstanceData_;
};

}  // namespace GraphicsWork

#endif  //! OCEAN_H