
#include <algorithm>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION

#include "CmdBufHandler.h"
#include "MyShell.h"
#include "Texture.h"

#define STB_FORMAT VK_FORMAT_R8G8B8A8_UNORM

std::future<std::shared_ptr<Texture::TextureData>> Texture::loadTexture(const VkDevice& dev, const bool makeMipmaps,
                                                                        std::shared_ptr<TextureData> pTex) {
    pTex->pLdgRes = LoadingResourceHandler::createLoadingResources();

    return std::async(std::launch::async, [&dev, &makeMipmaps, pTex]() {
        int width, height, channels;

        // Diffuse map (default)
        // TODO: make this dynamic like the others...
        pTex->pixels = stbi_load(pTex->path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pTex->pixels) {
            throw std::runtime_error("failed to load texture map!");
        }

        pTex->width = static_cast<uint32_t>(width);
        pTex->height = static_cast<uint32_t>(height);
        pTex->channels = static_cast<uint32_t>(channels);
        pTex->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(pTex->width, pTex->height)))) + 1;

        // Normal map
        if (!pTex->normPath.empty()) {
            pTex->normPixels = stbi_load(pTex->normPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!pTex->normPixels) {
                throw std::runtime_error("failed to load normal map!");
            }

            std::stringstream ss;
            if (width != pTex->width) ss << "invalid normal map (width)! ";
            if (height != pTex->height) ss << "invalid normal map (height)! ";
            if (channels != pTex->channels) ss << "invalid normal map (channels)! ";  // TODO: not sure about this
            if (!ss.str().empty()) {
                throw std::runtime_error(ss.str());
            }
        }

        // Spectral map
        if (!pTex->specPath.empty()) {
            pTex->specPixels = stbi_load(pTex->specPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!pTex->specPixels) {
                throw std::runtime_error("failed to load spectral map!");
            }

            std::stringstream ss;
            if (width != pTex->width) ss << "invalid spectral map (width)! ";
            if (height != pTex->height) ss << "invalid spectral map (height)! ";
            if (channels != pTex->channels) ss << "invalid spectral map (channels)! ";  // TODO: not sure about this
            if (!ss.str().empty()) {
                throw std::runtime_error(ss.str());
            }
        }

        return std::move(pTex);
    });
}

void Texture::createTexture(const VkDevice& dev, const bool makeMipmaps, std::shared_ptr<TextureData> pTex) {
    auto& tex = (*pTex);

    createImage(dev, tex);

    if (makeMipmaps) {
        generateMipmaps(tex);
    } else {
        // TODO: DO SOMETHING .. this was not tested
    }
    createImageView(dev, tex);
    createSampler(dev, tex);
    createDescInfo(tex);

    LoadingResourceHandler::loadSubmit(std::move(pTex->pLdgRes));

    pTex->status = Texture::STATUS::READY;
}

void Texture::createImage(const VkDevice& dev, TextureData& tex) {
    VkDeviceSize imageSize = tex.width * tex.height * 4;  // TODO: where does this 4 come from?!!!!!!!!!!!!!!
    auto layerCount = getArrayLayerCount(tex);

    BufferResource stgRes = {};
    auto memReqsSize = helpers::createBuffer(dev, (imageSize * layerCount), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             stgRes.buffer, stgRes.memory);

    void* pData;
    size_t offset = 0;
    vkMapMemory(dev, stgRes.memory, 0, memReqsSize, 0, &pData);

    if (tex.flags & FLAGS::DIFFUSE) {
        memcpy(static_cast<char*>(pData) + offset, tex.pixels, static_cast<size_t>(imageSize));
        offset += static_cast<size_t>(imageSize);
    }
    if (tex.flags & FLAGS::NORMAL) {
        memcpy(static_cast<char*>(pData) + offset, tex.pixels, static_cast<size_t>(imageSize));
        offset += static_cast<size_t>(imageSize);  // TODO: same size for all?
    }
    if (tex.flags & FLAGS::SPECULAR) {
        memcpy(static_cast<char*>(pData) + offset, tex.pixels, static_cast<size_t>(imageSize));
        offset += static_cast<size_t>(imageSize);  // TODO: same size for all?
    }

    vkUnmapMemory(dev, stgRes.memory);

    stbi_image_free(tex.pixels);
    stbi_image_free(tex.normPixels);
    stbi_image_free(tex.specPixels);

    // Using CmdBufHandler::getUniqueQueueFamilies(true, false, true) here might not be wise... To work
    // right it relies on the the two command buffers being created with the same data.
    helpers::createImage(dev, CmdBufHandler::getUniqueQueueFamilies(true, false, true), VK_SAMPLE_COUNT_1_BIT, STB_FORMAT,
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, static_cast<uint32_t>(tex.width), static_cast<uint32_t>(tex.height),
                         tex.mipLevels, layerCount, tex.image, tex.memory);

    helpers::transitionImageLayout(tex.pLdgRes->transferCmd, tex.image, STB_FORMAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, tex.mipLevels, layerCount);

    helpers::copyBufferToImage(tex.pLdgRes->graphicsCmd, tex.width, tex.height, layerCount, stgRes.buffer, tex.image);

    tex.pLdgRes->stgResources.push_back(stgRes);
}

void Texture::generateMipmaps(const TextureData& tex) {
    // This was the way before mip maps
    // transitionImageLayout(
    //    srcQueueFamilyIndexFinal,
    //    dstQueueFamilyIndexFinal,
    //    m_mipLevels,
    //    image,
    //    VK_FORMAT_R8G8B8A8_UNORM,
    //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //    m_transferCommandPool
    //);

    // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = tex.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = tex.width;
    int32_t mipHeight = tex.height;

    for (uint32_t i = 1; i < tex.mipLevels; i++) {
        // CREATE MIP LEVEL

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(tex.pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(tex.pLdgRes->graphicsCmd, tex.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tex.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // TRANSITION TO SHADER READY

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(tex.pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        // This is a bit wonky methinks (non-sqaure is the case for this)
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // This is not handled in the loop so one more!!!! The last level is never never
    // blitted from.

    barrier.subresourceRange.baseMipLevel = tex.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(tex.pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
}

void Texture::createImageView(const VkDevice& dev, TextureData& tex) {
    helpers::createImageView(dev, tex.image, tex.mipLevels, STB_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                             getArrayLayerCount(tex), tex.view);
}

void Texture::createSampler(const VkDevice& dev, TextureData& tex) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;  // TODO: OPTION (FEATURE BASED)
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;  // test this out for fun
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0;  // static_cast<float>(m_mipLevels / 2); // Optional
    samplerInfo.maxLod = static_cast<float>(tex.mipLevels);
    samplerInfo.mipLodBias = 0;  // Optional

    vk::assert_success(vkCreateSampler(dev, &samplerInfo, nullptr, &tex.sampler));

    // Name some objects for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)tex.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,
                                  (tex.name + " sampler").c_str());
}

void Texture::createDescInfo(TextureData& tex) {
    tex.imgDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    tex.imgDescInfo.imageView = tex.view;
    tex.imgDescInfo.sampler = tex.sampler;
}

uint32_t Texture::getArrayLayerCount(const TextureData& tex) {
    uint32_t count = 0;
    if (tex.flags & FLAGS::DIFFUSE) count++;
    if (tex.flags & FLAGS::NORMAL) count++;
    if (tex.flags & FLAGS::SPECULAR) count++;
    return count;
}