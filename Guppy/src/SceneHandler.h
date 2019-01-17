#ifndef SCENE_HANDLER_H
#define SCENE_HANDLER_H

#include <memory>
#include <vector>

#include "Constants.h"
#include "Helpers.h"
#include "Singleton.h"
#include "Scene.h"
#include "Shell.h"
#include "UniformBuffer.h"

class ColorMesh;
class LineMesh;
class TextureMesh;

struct InvalidSceneResources {
    VkDescriptorPool pool = VK_NULL_HANDLE;
    std::vector<std::vector<VkDescriptorSet>> texSets = {};
    std::shared_ptr<DescriptorBufferResources> pDynUBORes = nullptr;
};

class SceneHandler : public Singleton {
    friend class Scene;

   public:
    class SelectionManager;

    static void init(Shell* sh, const Game::Settings& settings);
    static inline void destroy() {
        inst_.reset();
        inst_.cleanupInvalidResources();
    }

    static SCENE_INDEX_TYPE makeScene(bool setActive = false, bool makeFaceSelection = false);

    static std::unique_ptr<Scene>& getActiveScene() {
        assert(inst_.activeSceneIndex_ < inst_.pScenes_.size());
        return inst_.pScenes_[inst_.activeSceneIndex_];
    }

    static std::unique_ptr<ColorMesh>& getColorMesh(size_t sceneOffset, size_t meshOffset) {
        return inst_.pScenes_[sceneOffset]->getColorMesh(meshOffset);
    }
    static std::unique_ptr<LineMesh>& getLineMesh(size_t sceneOffset, size_t meshOffset) {
        return inst_.pScenes_[sceneOffset]->getLineMesh(meshOffset);
    }
    static std::unique_ptr<TextureMesh>& getTextureMesh(size_t sceneOffset, size_t meshOffset) {
        return inst_.pScenes_[sceneOffset]->getTextureMesh(meshOffset);
    }

    // PIPELINE
    static void updatePipelineReferences(const PIPELINE_TYPE& type, const VkPipeline& pipeline);

    // SELECTION
    static const std::unique_ptr<Face>& getFaceSelectionFace();
    static void select(const VkDevice& dev, const Ray& ray);

    static void updateDescriptorSets(SCENE_INDEX_TYPE offset, bool isUpdate = true);
    static void cleanupInvalidResources();

   private:
    SceneHandler();   // Prevent construction
    ~SceneHandler();  // Prevent construction
    static SceneHandler inst_;
    void reset() override;

    Shell* sh_;
    Shell::Context ctx_;       // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    SCENE_INDEX_TYPE activeSceneIndex_;
    std::vector<std::unique_ptr<Scene>> pScenes_;

    // global storage for clean up
    std::vector<InvalidSceneResources> invalidRes_;

    // Selection
    std::unique_ptr<SelectionManager> pSelectionManager_;
};

#endif  // !SCENE_HANDLER_H
