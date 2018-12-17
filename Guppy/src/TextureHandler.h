#ifndef TEXTURE_HANDLER_H
#define TEXTURE_HANDLER_H

#include <vector>

#include "Singleton.h"
#include "Texture.h"

class Shell;

class TextureHandler : Singleton {
   public:
    static void init(Shell* sh);
    static inline void destroy() { inst_.reset(); }

    static void addTexture(std::string path, std::string normPath = "", std::string specPath = "");
    static std::shared_ptr<Texture::Data> getTextureByPath(std::string path);
    static void update();

    static const std::vector<std::shared_ptr<Texture::Data>>& getTextures() { return inst_.pTextures_; }
    static uint32_t getCount() { return static_cast<uint32_t>(inst_.pTextures_.size()); }

   private:
    TextureHandler() : sh_(nullptr){};  // Prevent construction
    ~TextureHandler(){};                // Prevent construction
    static TextureHandler inst_;
    void reset() override;

    Shell* sh_;  // TODO: shared_ptr

    std::vector<std::shared_ptr<Texture::Data>> pTextures_;
    std::vector<std::future<std::shared_ptr<Texture::Data>>> texFutures_;
};

#endif  // !TEXTURE_HANDLER_H