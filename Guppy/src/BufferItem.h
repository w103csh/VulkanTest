/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef BUFFER_ITEM_H
#define BUFFER_ITEM_H

#include <assert.h>
#include <functional>

#include <vulkan/vulkan.hpp>

namespace Buffer {

struct CreateInfo {
    uint32_t dataCount = 1;
    bool update = true;
    bool countInRange = false;
};

struct Info {
    vk::DescriptorBufferInfo bufferInfo;
    uint32_t count;
    vk::DeviceSize dataOffset;
    vk::DeviceSize itemOffset;
    vk::DeviceSize memoryOffset;
    vk::DeviceSize resourcesOffset;
};

class Item {
   public:
    Item(const Buffer::Info&& info)  //
        : BUFFER_INFO(info), dirty(false) {
        assert(BUFFER_INFO.bufferInfo.buffer);
    }
    virtual ~Item() = default;

    const Buffer::Info BUFFER_INFO;
    bool dirty;

   protected:
    /** Virtual inheritance only.
     *   I am going to assert here to make it clear that its best to avoid this
     *   constructor being called. It gave me some headaches, so this will hopefully
     *   force me to call the constructors in the same order. The order is that "the
     *   most derived class calls the constructor" of the virtually inherited
     *   class (this), so just add the public constructor above to that level.
     */
    Item() : BUFFER_INFO(), dirty(false) { assert(false); }
};

template <typename TDATA>
class DataItem : public virtual Buffer::Item {
   public:
    using DATA = TDATA;

    DataItem(TDATA* pData) : pData_(pData) { assert(pData_); }

    virtual void setData(const uint32_t index = 0) {}

   protected:
    TDATA* pData_;
};

// This is pretty slow on a bunch of levels, but I'd rather just have it work atm.
template <typename TDATA>
class PerFramebufferDataItem : public Buffer::DataItem<TDATA> {
    using TItem = Buffer::DataItem<TDATA>;

   public:
    PerFramebufferDataItem(TDATA* pData) : Buffer::DataItem<TDATA>(pData), data_(*pData) {}

   protected:
    void setData(const uint32_t index = UINT32_MAX) override {
        if (index == UINT32_MAX) {
            for (uint32_t i = 0; i < Item::BUFFER_INFO.count; i++)
                std::memcpy(PerFramebufferDataItem::getData(i * TItem::BUFFER_INFO.bufferInfo.range), &data_, sizeof(TDATA));
        } else {
            assert(index < Item::BUFFER_INFO.count);
            std::memcpy(PerFramebufferDataItem::getData(index * TItem::BUFFER_INFO.bufferInfo.range), &data_, sizeof(TDATA));
        }
        Item::dirty = true;
    }

    TDATA data_;

   private:
    inline void* getData(const vk::DeviceSize offset) { return (((uint8_t*)TItem::pData_) + offset); }
};

}  // namespace Buffer

#endif  // !BUFFER_ITEM_H
