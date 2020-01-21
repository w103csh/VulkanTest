/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OCEAN_H
#define OCEAN_H

#include <glm/glm.hpp>
#include <string_view>

#include "ConstantsAll.h"
#include "HeightFieldFluid.h"
#include "ParticleBuffer.h"
#include "Pipeline.h"

namespace Ocean {

constexpr float T = 200.0f;  // wave repeat time
constexpr float g = 9.81f;   // gravity

struct SurfaceCreateInfo {
    SurfaceCreateInfo()
        : Lx(1000.0f),  //
          Lz(1000.0f),
          N(512),
          M(512),
          V(31.0f),
          omega(1.0f, 0.0f),
          l(1.0f),
          A(2e-5f),
          L() {
        L = (V * V) / 9.81f;
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
};

}  // namespace Ocean

// TEXTURE
namespace Texture {
class Handler;
struct CreateInfo;
namespace Ocean {

constexpr std::string_view OCEAN_DATA_ID = "Ocean Data Texture";
void MakTextures(Handler& handler, const ::Ocean::SurfaceCreateInfo& info);

}  // namespace Ocean
}  // namespace Texture

// SHADER
namespace Shader {
namespace Ocean {
extern const CreateInfo DISP_COMP_CREATE_INFO;
extern const CreateInfo FFT_COMP_CREATE_INFO;
extern const CreateInfo VERT_CREATE_INFO;
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {

namespace Simulation {
struct DATA {
    uint32_t nLog2, mLog2;  // log2 of discrete dimensions
    float t;                // time
};
struct CreateInfo : Buffer::CreateInfo {
    ::Ocean::SurfaceCreateInfo info;
};
class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
};
}  // namespace Simulation

}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo OCEAN_DEFAULT_CREATE_INFO;
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
class Handler;
namespace Ocean {

// DISPERSION
class Dispersion : public Compute {
   public:
    Dispersion(Handler& handler);

   private:
    void getShaderStageInfoResources(CreateInfoResources& createInfoRes);

    float omega0_;
};

// FFT
class FFT : public Compute {
   public:
    FFT(Handler& handler);
};

// WIREFRAME
class Wireframe : public Graphics {
   public:
    const bool DO_BLEND;
    const bool IS_DEFERRED;

    Wireframe(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Ocean
}  // namespace Pipeline

// BUFFER
namespace Ocean {

struct CreateInfo : Particle::Buffer::CreateInfo {
    SurfaceCreateInfo info;
};

class Buffer : public Particle::Buffer::Base, public Obj3d::InstanceDraw {
   public:
    Buffer(Particle::Handler& handler, const Particle::Buffer::index&& offset, const CreateInfo* pCreateInfo,
           std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
           std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData);

    virtual void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                      const uint8_t frameIndex) const override;
    void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                  const uint8_t frameIndex) const override;

    GRAPHICS drawMode;

   private:
    void loadBuffers() override;
    void destroy() override;

    uint32_t normalOffset_;

    std::vector<HeightFieldFluid::VertexData> verticesHFF_;
    BufferResource verticesHFFRes_;
    std::vector<VB_INDEX_TYPE> indicesWF_;
    BufferResource indexWFRes_;
};

}  // namespace Ocean

#endif  //! OCEAN_H