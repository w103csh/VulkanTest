#ifndef LDG_RESOURCE_HANDLER_H
#define LDG_RESOURCE_HANDLER_H

#include "Helpers.h"
#include <vector>
#include <vulkan\vulkan.h>

struct LoadingResources {
    LoadingResources() : shouldWait(false) {};
    bool shouldWait;
    VkCommandBuffer graphicsCmd, transferCmd;
    std::vector<BufferResource> stgResources;
    std::vector<VkFence> fences;
    VkSemaphore semaphore;
    bool cleanup(const VkDevice &dev);
};

class LoadingResourceHandler {
   public:
    static void init(const MyShell::Context &ctx);

    LoadingResourceHandler(const LoadingResourceHandler &) = delete;             // Prevent construction by copying
    LoadingResourceHandler &operator=(const LoadingResourceHandler &) = delete;  // Prevent assignment

    static std::unique_ptr<LoadingResources> createLoadingResources();
    static void loadSubmit(std::unique_ptr<LoadingResources> pLdgRes);
    static void cleanupResources();

   private:
    LoadingResourceHandler();  // Prevent construction
    static LoadingResourceHandler inst_;

    MyShell::Context ctx_;  // TODO: shared_ptr

    std::vector<std::unique_ptr<LoadingResources>> ldgResources_;
};

#endif  // !LDG_RESOURCE_HANDLER_H