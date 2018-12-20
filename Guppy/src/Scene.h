
#ifndef SCENE_H
#define SCENE_H

#include <array>
#include <future>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Mesh.h"
#include "Shell.h"

class Scene {
    friend class SceneHandler;

   public:
    std::unique_ptr<ColorMesh> &getColorMesh(size_t index) { return colorMeshes_[index]; }
    std::unique_ptr<ColorMesh> &getLineMesh(size_t index) { return lineMeshes_[index]; }
    std::unique_ptr<TextureMesh> &getTextureMesh(size_t index) { return texMeshes_[index]; }

    inline size_t getOffset() { return offset_; }

    // TODO: there is too much redundancy here...

    std::unique_ptr<ColorMesh> &moveMesh(const Shell::Context &ctx, std::unique_ptr<ColorMesh> pMesh);
    std::unique_ptr<ColorMesh> &moveMesh(const Shell::Context &ctx, std::unique_ptr<LineMesh> pMesh);
    std::unique_ptr<TextureMesh> &moveMesh(const Shell::Context &ctx, std::unique_ptr<TextureMesh> pMesh);
    size_t addMesh(const Shell::Context &ctx, std::unique_ptr<ColorMesh> pMesh, bool async = true,
                   std::function<void(Mesh *)> callback = nullptr);
    size_t addMesh(const Shell::Context &ctx, std::unique_ptr<LineMesh> pMesh, bool async = true,
                   std::function<void(Mesh *)> callback = nullptr);
    size_t addMesh(const Shell::Context &ctx, std::unique_ptr<TextureMesh> pMesh, bool async = true,
                   std::function<void(Mesh *)> callback = nullptr);

    void removeMesh(std::unique_ptr<Mesh> &pMesh);

    inline const VkRenderPass activeRenderPass() const { return plResources_.renderPass; }

    void record(const Shell::Context &ctx, const VkFramebuffer &framebuffer, const VkFence &fence,
                const VkViewport &viewport, const VkRect2D &scissor, uint8_t frameIndex, bool wait = false);

    void update(const Shell::Context &ctx);

    const VkCommandBuffer &getDrawCmds(const uint8_t &frame_data_index, uint32_t &commandBufferCount);

    // Selection
    void select(const Ray &ray);

    void destroy(const VkDevice &dev);

    // TODO: not sure about these being here...
    inline const VkPipeline &getPipeline(PIPELINE_TYPE type) { return plResources_.pipelines[static_cast<int>(type)]; }
    void updatePipeline(PIPELINE_TYPE type);

   private:
    Scene() = delete;
    Scene(size_t offset);

    size_t offset_;

    // Descriptor
    inline bool isNewTexture(std::unique_ptr<TextureMesh> &pMesh) const {
        return !(pMesh->getTextureOffset() < pDescResources_->texSets.size());
    }
    inline const VkDescriptorSet &getColorDescSet(int frameIndex) { return pDescResources_->colorSets[frameIndex]; }
    inline const VkDescriptorSet &getTexDescSet(size_t offset, int frameIndex) {
        return pDescResources_->texSets[offset][frameIndex];
    }
    std::unique_ptr<DescriptorResources> pDescResources_;

    // Pipeline
    PipelineResources plResources_;

    // Draw commands
    void createDrawCmds(const Shell::Context &ctx);
    void destroyCmds(const VkDevice &dev);
    std::vector<DrawResources> draw_resources_;

    // Meshes
    // color
    std::vector<std::unique_ptr<ColorMesh>> colorMeshes_;  // Vertex::TYPE::COLOR, PIPELINE_TYPE::TRI_LIST_COLOR
    std::vector<std::unique_ptr<ColorMesh>> lineMeshes_;   // Vertex::TYPE::COLOR, PIPELINE_TYPE::LINE
    // texture
    std::vector<std::unique_ptr<TextureMesh>> texMeshes_;  // Vertex::TYPE::TEX, PIPELINE_TYPE::TRI_LIST_TEX

    // Loading
    std::vector<std::future<Mesh *>> ldgFutures_;
    std::vector<std::unique_ptr<LoadingResources>> ldgResources_;

    // Uniform buffer
    void createDynamicTexUniformBuffer(const Shell::Context &ctx, const Game::Settings &settings,
                                       std::string markerName = "");
    void destroyUniforms(const VkDevice &dev);
    std::shared_ptr<UniformBufferResources> pDynUBOResource_;

    // Selection
    template <typename T>
    void selectFace(const Ray &ray, float &tMin, T &pMeshes, Face &face) {
        for (size_t offset = 0; offset < pMeshes.size(); offset++) {
            const auto &pMesh = pMeshes[offset];
            if (pMesh->isSelectable()) {
                if (pMesh->testBoundingBox(ray.e, ray.d, tMin)) {
                    pMesh->selectFace(ray, tMin, face, offset);
                }
            }
        }
    }
};

#endif  // !SCENE_H