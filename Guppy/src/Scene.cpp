/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Scene.h"

#include <iterator>
#include <variant>

#include "Face.h"
#include "RenderPass.h"
#include "Shell.h"
// HANDLERS
#include "MeshHandler.h"
#include "ModelHandler.h"
#include "PipelineHandler.h"
#include "SceneHandler.h"
#include "SelectionManager.h"
#include "TextureHandler.h"

namespace {

struct UBOTag {
    const char name[17] = "ubo tag";
} uboTag;

}  // namespace

Scene::Base::Base(Scene::Handler& handler, const index offset, bool makeFaceSelection)
    : Handlee(handler),
      OFFSET(offset),
      moonOffset(Mesh::BAD_OFFSET),
      starsOffset(Mesh::BAD_OFFSET),
      posLgtCubeShdwOffset(Mesh::BAD_OFFSET),
      debugFrustumOffset(Mesh::BAD_OFFSET),
      pSelectionManager_(std::make_unique<Selection::Manager>(handler, makeFaceSelection)) {}

Scene::Base::~Base() = default;

void Scene::Base::addMeshIndex(const MESH type, const Mesh::index offset) {
    if (!handler().meshHandler().checkOffset(type, offset)) {
        assert(false);
        exit(EXIT_FAILURE);
    }
    switch (type) {
        case MESH::COLOR: {
            colorOffsets_.insert(offset);
        } break;
        case MESH::LINE: {
            lineOffsets_.insert(offset);
        } break;
        case MESH::TEXTURE: {
            texOffsets_.insert(offset);
        } break;
        default: {
            assert(false);
            exit(EXIT_FAILURE);
        }
    }
}

void Scene::Base::addModelIndex(const Model::index offset) {
    if (!handler().modelHandler().checkOffset(offset)) {
        assert(false);
        exit(EXIT_FAILURE);
    }
    assert(modelOffsets_.count(offset) == 0);
    modelOffsets_.insert(offset);
}

void Scene::Base::record(const RENDER_PASS& passType, const PIPELINE& pipelineType,
                         const std::shared_ptr<Pipeline::BindData>& pPipelineBindData, const vk::CommandBuffer& priCmd,
                         const vk::CommandBuffer& secCmd, const uint8_t frameIndex,
                         const Descriptor::Set::BindData* pDescSetBindData) {
    switch (std::visit(Pipeline::GetGraphics{}, pipelineType)) {
        case GRAPHICS::DEFERRED_MRT_COLOR:
        case GRAPHICS::DEFERRED_MRT_WF_COLOR:
        case GRAPHICS::DEFERRED_MRT_PT:
        case GRAPHICS::DEFERRED_MRT_COLOR_RFL_RFR:
        case GRAPHICS::DEFERRED_MRT_SKYBOX:
        case GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED:
        case GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED:
        case GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED:
        case GRAPHICS::PRTCL_WAVE_DEFERRED:
        case GRAPHICS::PBR_COLOR:
        case GRAPHICS::CUBE:
        case GRAPHICS::TRI_LIST_COLOR: {
            for (const auto& offset : colorOffsets_) {
                auto& pMesh = handler().meshHandler().getColorMesh(offset);
                if (pMesh->shouldDraw(passType, pipelineType)) {
                    if (pDescSetBindData == nullptr)
                        pMesh->draw(passType, pPipelineBindData, priCmd, frameIndex);
                    else
                        pMesh->draw(passType, pPipelineBindData, *pDescSetBindData, priCmd, frameIndex);
                }
            }
            for (const auto& modelOffset : modelOffsets_) {
                for (const auto& offset : handler().modelHandler().getModel(modelOffset)->getMeshOffsets(MESH::COLOR)) {
                    auto& pMesh = handler().meshHandler().getColorMesh(offset);
                    if (pMesh->shouldDraw(passType, pipelineType)) {
                        if (pDescSetBindData == nullptr)
                            pMesh->draw(passType, pPipelineBindData, priCmd, frameIndex);
                        else
                            pMesh->draw(passType, pPipelineBindData, *pDescSetBindData, priCmd, frameIndex);
                    }
                }
            }
        } break;
        case GRAPHICS::DEFERRED_MRT_TEX:
        case GRAPHICS::PARALLAX_SIMPLE:
        case GRAPHICS::PARALLAX_STEEP:
        case GRAPHICS::PBR_TEX:
        case GRAPHICS::BP_TEX_CULL_NONE:
        case GRAPHICS::TRI_LIST_TEX: {
            for (const auto& offset : texOffsets_) {
                auto& pMesh = handler().meshHandler().getTextureMesh(offset);
                if (pMesh->shouldDraw(passType, pipelineType)) {
                    if (pDescSetBindData == nullptr)
                        pMesh->draw(passType, pPipelineBindData, priCmd, frameIndex);
                    else
                        pMesh->draw(passType, pPipelineBindData, *pDescSetBindData, priCmd, frameIndex);
                }
            }
            for (const auto& modelOffset : modelOffsets_) {
                for (const auto& offset : handler().modelHandler().getModel(modelOffset)->getMeshOffsets(MESH::TEXTURE)) {
                    auto& pMesh = handler().meshHandler().getTextureMesh(offset);
                    if (pMesh->shouldDraw(passType, pipelineType)) {
                        if (pDescSetBindData == nullptr)
                            pMesh->draw(passType, pPipelineBindData, priCmd, frameIndex);
                        else
                            pMesh->draw(passType, pPipelineBindData, *pDescSetBindData, priCmd, frameIndex);
                    }
                }
            }
        } break;
        case GRAPHICS::DEFERRED_MRT_LINE:
        case GRAPHICS::TESS_BEZIER_4_DEFERRED:
        case GRAPHICS::LINE: {
            for (const auto& offset : lineOffsets_) {
                auto& pMesh = handler().meshHandler().getLineMesh(offset);
                if (pMesh->shouldDraw(passType, pipelineType)) {
                    if (pDescSetBindData == nullptr)
                        pMesh->draw(passType, pPipelineBindData, priCmd, frameIndex);
                    else
                        pMesh->draw(passType, pPipelineBindData, *pDescSetBindData, priCmd, frameIndex);
                }
            }
            for (const auto& modelOffset : modelOffsets_) {
                for (const auto& offset : handler().modelHandler().getModel(modelOffset)->getMeshOffsets(MESH::LINE)) {
                    auto& pMesh = handler().meshHandler().getLineMesh(offset);
                    if (pMesh->shouldDraw(passType, pipelineType)) {
                        if (pDescSetBindData == nullptr)
                            pMesh->draw(passType, pPipelineBindData, priCmd, frameIndex);
                        else
                            pMesh->draw(passType, pPipelineBindData, *pDescSetBindData, priCmd, frameIndex);
                    }
                }
            }
        } break;
        default:;
    }
}

void Scene::Base::select(const Ray& ray) {
    if (!pSelectionManager_->isEnabled()) return;

    float tMin = T_MAX;  // This is relative to the distance between ray.e & ray.d
    Face face;

    // TODO: pass the scene in, or better, fix it...
    pSelectionManager_->selectFace(ray, tMin, handler().meshHandler().getColorMeshes(), face);
    pSelectionManager_->selectFace(ray, tMin, handler().meshHandler().getTextureMeshes(), face);

    pSelectionManager_->updateFace((tMin < T_MAX) ? std::make_unique<Face>(face) : nullptr);
}

void Scene::Base::destroy() {}