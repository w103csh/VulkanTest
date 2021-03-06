/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Mesh.h"

#include <variant>

#include "Face.h"
#include "FileLoader.h"
#include "PBR.h"  // TODO: this is bad
// HANDLERS
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "MeshHandler.h"
#include "PipelineHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

// BASE

Mesh::Base::Base(Mesh::Handler& handler, const index&& offset, const MESH&& type, const VERTEX&& vertexType,
                 const std::string&& name, const CreateInfo* pCreateInfo,
                 std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Handlee(handler),
      Obj3d::InstanceDraw(pInstanceData),
      MAPPABLE(pCreateInfo->mappable),
      NAME(name),
      PASS_TYPES(pCreateInfo->passTypes),
      PIPELINE_TYPE(pCreateInfo->pipelineType),
      SETTINGS(pCreateInfo->settings),
      TYPE(type),
      VERTEX_TYPE(vertexType),
      status_(STATUS::PENDING),
      // INFO
      selectable_(pCreateInfo->selectable),
      //
      vertexRes_(),
      indexRes_(),
      pLdgRes_(nullptr),
      pMaterial_(pMaterial),
      offset_(offset)
//
{
    if (PIPELINE_TYPE == PIPELINE{GRAPHICS::ALL_ENUM} && PASS_TYPES.empty()) {
        /* The only mesh that should do this currently is actually a VERTEX::SCREEN_QUAD and there
         *  should only be one of them, but I don't feel like doing all the work to enforce everything.
         *  This type should also have no descriptor bind data or a material if I ever get around to
         *  cleaning it up.
         *
         * Note: The PASS_TYPES being empty is extra validation that is meaningless and misleading, but
         *  I want the asserts in the else to keep working.
         */
        assert(VERTEX_TYPE == VERTEX::TEXTURE);
    } else {
        assert(PASS_TYPES.size());
        assert(!std::visit(Pipeline::IsAll{}, PIPELINE_TYPE));
        assert(Mesh::Base::handler().pipelineHandler().checkVertexPipelineMap(VERTEX_TYPE, PIPELINE_TYPE));
    }
    // Warn that this won't do anything, and functionality probably needs to be added.
    for (const auto& passType : PASS_TYPES) {
        if (std::visit(Pass::IsCompute{}, passType)) {
            assert(std::visit(Pipeline::IsCompute{}, PIPELINE_TYPE));
        }
    }
    assert(pInstObj3d_ != nullptr);
    assert(pMaterial_ != nullptr);
}

Mesh::Base::~Base() = default;

void Mesh::Base::prepare() {
    assert(status_ ^ STATUS::READY);

    if (SETTINGS.needAdjacenyList) makeAdjacenyList();

    if (status_ == STATUS::PENDING_BUFFERS) {
        loadBuffers();
        // Submit vertex loading commands...
        handler().loadingHandler().loadSubmit(std::move(pLdgRes_));
        // Screen quad mesh only needs vertex/index buffers
        if (PIPELINE_TYPE == PIPELINE{GRAPHICS::ALL_ENUM} && PASS_TYPES.empty())
            status_ = STATUS::READY;
        else
            status_ = STATUS::PENDING_MATERIAL | STATUS::PENDING_PIPELINE;
    }

    if (status_ & STATUS::PENDING_MATERIAL) {
        if (pMaterial_->getStatus() == STATUS::READY) {
            status_ ^= STATUS::PENDING_MATERIAL;
        }
    }

    if (status_ & STATUS::PENDING_PIPELINE) {
        auto& pPipeline = handler().pipelineHandler().getPipeline(PIPELINE_TYPE);
        if (pPipeline->getStatus() == STATUS::READY)
            status_ ^= STATUS::PENDING_PIPELINE;
        else
            pPipeline->updateStatus();
    }

    /* TODO: apparently you can allocate the descriptor sets immediately, and then
     *  you just need to wait to use the sets until they have been updated with valid
     *  resources... I should implement this. As of now just wait until we
     *  can allocate and update all at once.
     */
    if (status_ == STATUS::READY) {
        // Screen quad mesh will not keep track of descriptor data.
        if (PIPELINE_TYPE == PIPELINE{GRAPHICS::ALL_ENUM} && PASS_TYPES.empty()) return;

        handler().descriptorHandler().getBindData(PIPELINE_TYPE, descSetBindDataMap_, {pMaterial_.get()});
    } else {
        handler().ldgOffsets_.insert({TYPE, getOffset()});
    }
}

void Mesh::Base::makeAdjacenyList() {
    // Only have done triangle adjacency so far.
    assert(TYPE == MESH::COLOR || TYPE == MESH::TEXTURE);
    helpers::makeTriangleAdjacenyList(indices_, indicesAdjaceny_);
}

const Descriptor::Set::BindData& Mesh::Base::getDescriptorSetBindData(const PASS& passType) const {
    for (const auto& [passTypes, bindData] : descSetBindDataMap_) {
        if (passTypes.find(passType) != passTypes.end()) return bindData;
    }
    return descSetBindDataMap_.at(Uniform::PASS_ALL_SET);
}

// thread sync
void Mesh::Base::loadBuffers() {
    assert(getVertexCount());

    const auto& ctx = handler().shell().context();
    pLdgRes_ = handler().loadingHandler().createLoadingResources();

    vk::BufferUsageFlags vertexUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;

    // Vertex buffer
    BufferResource stgRes = {};
    ctx.createBuffer(pLdgRes_->transferCmd, vertexUsage, getVertexBufferSize(), NAME + " vertex", stgRes, vertexRes_,
                     getVertexData(), MAPPABLE);
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    vk::BufferUsageFlags indexUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;

    // Index buffer
    if (getIndexCount()) {
        stgRes = {};
        ctx.createBuffer(pLdgRes_->transferCmd, indexUsage, getIndexBufferSize(), NAME + " index", stgRes, indexRes_,
                         getIndexData(), MAPPABLE);
        pLdgRes_->stgResources.push_back(std::move(stgRes));
    }

    // Index adjacency buffer
    if (indicesAdjaceny_.size()) {
        // TODO: I should probably either create this buffer or the normal index buffer. If you
        // update this then you should also do this everywhere like "updateBuffers" for example.
        stgRes = {};
        ctx.createBuffer(pLdgRes_->transferCmd, indexUsage, getIndexBufferAdjSize(), NAME + " adjacency index", stgRes,
                         indexAdjacencyRes_, indicesAdjaceny_.data(), MAPPABLE);
        pLdgRes_->stgResources.push_back(std::move(stgRes));
    }
}

void Mesh::Base::createBufferData(const vk::CommandBuffer& cmd, BufferResource& stgRes, vk::DeviceSize bufferSize,
                                  const void* data, BufferResource& res, vk::BufferUsageFlagBits usage,
                                  std::string bufferType) {
    const auto& ctx = handler().shell().context();

    // STAGING RESOURCE
    res.memoryRequirements.size =
        helpers::createBuffer(ctx.dev, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                              ctx.memProps, stgRes.buffer, stgRes.memory, ctx.pAllocator);

    // FILL STAGING BUFFER ON DEVICE
    void* pData = ctx.dev.mapMemory(stgRes.memory, 0, res.memoryRequirements.size);
    /*
     *  You can now simply memcpy the vertex data to the mapped memory and unmap it again using unmapMemory.
     *  Unfortunately the driver may not immediately copy the data into the buffer memory, for example because
     *  of caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. There
     *  are two ways to deal with that problem:
     *      - Use a memory heap that is host coherent, indicated with vk::MemoryPropertyFlagBits::eHostCoherent
     *      - Call flushMappedMemoryRanges to after writing to the mapped memory, and call
     *        invalidateMappedMemoryRanges before reading from the mapped memory
     *  We went for the first approach, which ensures that the mapped memory always matches the contents of the
     *  allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit flushing,
     *  but we'll see why that doesn't matter in the next chapter.
     */
    memcpy(pData, data, static_cast<size_t>(bufferSize));
    ctx.dev.unmapMemory(stgRes.memory);

    // FAST VERTEX BUFFER
    vk::MemoryPropertyFlags memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
    if (MAPPABLE) memProps |= vk::MemoryPropertyFlagBits::eHostVisible;
    helpers::createBuffer(ctx.dev, bufferSize,
                          // TODO: probably don't need to check memory requirements again
                          usage, memProps, ctx.memProps, res.buffer, res.memory, ctx.pAllocator);

    // COPY FROM STAGING TO FAST
    helpers::copyBuffer(cmd, stgRes.buffer, res.buffer, res.memoryRequirements.size);
    std::string markerName = NAME + " " + bufferType + " mesh";
    // ctx.dbg.setMarkerName(res.buffer, markerName.c_str());
}

void Mesh::Base::addVertex(const Face& face) {
    for (uint8_t i = 0; i < Face::NUM_VERTICES; i++) {
        auto index = face.getIndex(i);
        addVertex(face[i], index == BAD_VB_INDEX ? -1 : index);
    }
}

void Mesh::Base::updateBuffers() {
    auto& dev = handler().shell().context().dev;

    // VERTEX BUFFER
    vk::DeviceSize bufferSize = getVertexBufferSize(true);
    void* pData = dev.mapMemory(vertexRes_.memory, 0, VK_WHOLE_SIZE);
    memcpy(pData, getVertexData(), static_cast<size_t>(bufferSize));
    dev.unmapMemory(vertexRes_.memory);

    // INDEX BUFFER
    if (getIndexCount()) {
        bufferSize = getIndexBufferSize(true);
        pData = dev.mapMemory(indexRes_.memory, 0, indexRes_.memoryRequirements.size);
        memcpy(pData, getIndexData(), static_cast<size_t>(bufferSize));
        dev.unmapMemory(indexRes_.memory);
    }

    // INDEX BUFFER (ADJACENCY)
    if (indicesAdjaceny_.size()) {
        bufferSize = getIndexBufferAdjSize(true);
        pData = dev.mapMemory(indexAdjacencyRes_.memory, 0, indexAdjacencyRes_.memoryRequirements.size);
        memcpy(pData, indicesAdjaceny_.data(), static_cast<size_t>(bufferSize));
        dev.unmapMemory(indexAdjacencyRes_.memory);
    }
}

uint32_t Mesh::Base::getFaceCount() const {
    assert(indices_.size() % Face::NUM_VERTICES == 0);
    return static_cast<uint32_t>(indices_.size()) / Face::NUM_VERTICES;
}

Face Mesh::Base::getFace(size_t faceIndex) {
    IndexBufferType idx0 = indices_[faceIndex + 0];
    IndexBufferType idx1 = indices_[faceIndex + 1];
    IndexBufferType idx2 = indices_[faceIndex + 2];
    return {getVertexComplete(idx0), getVertexComplete(idx1), getVertexComplete(idx2), idx0, idx1, idx2, 0};
}

void Mesh::Base::selectFace(const Ray& ray, float& tMin, Face& face, size_t offset) const {
    bool hit = false;
    IndexBufferType idx0_hit = 0, idx1_hit = 0, idx2_hit = 0;

    // Declare some variables that will be reused
    float a, b, c;
    float d, e, f;
    // float g, h, i;
    float j, k, l;

    float ei_hf;
    float gf_di;
    float dh_eg;
    float ak_jb;
    float jc_al;
    float bl_kc;

    float beta, gamma, t, M;

    std::array<glm::vec4, 2> localRay = {glm::vec4(ray.e, 1.0f), glm::vec4(ray.d, 1.0f)};
    pInstObj3d_->worldToLocal(localRay);

    float t0 = 0.0f;  //, t1 = glm::distance(localRay[0], localRay[1]);  // TODO: like this???

    for (size_t n = 0; n < indices_.size(); n += 3) {
        // face indices
        const auto& idx0 = indices_[n];
        const auto& idx1 = indices_[n + 1];
        const auto& idx2 = indices_[n + 2];
        // face vertex positions (faces are only triangles for now)
        const auto& pa = getVertexPositionAtOffset(idx0);
        const auto& pb = getVertexPositionAtOffset(idx1);
        const auto& pc = getVertexPositionAtOffset(idx2);

        /*  This is solved using barycentric coordinates, and
            Cramer's rule.

            e + td = a + beta(b-a) + gamma(c-a)

            |xa-xb   xa-xc   xd| |b| = |xa-xe|
            |ya-yb   ya-yc   yd| |g| = |ya-ye|
            |za-zb   za-zc   zd| |t| = |za-ze|

            |a   d   g| |beta | = |j|
            |b   e   h| |gamma| = |k|
            |c   f   i| |t    | = |l|

            M =       a(ei-hf) + b(gf-di) + c(dh-eg)
            t =     (f(ak-jb) + e(jc-al) + d(bl-kc)) / -M
            gamma = (i(ak-jb) + h(jc-al) + g(bl-kc)) /  M
            beta =  (j(ei-hf) + k(gf-di) + l(dh-eg)) /  M
        */

        a = pa.x - pb.x;
        b = pa.y - pb.y;
        c = pa.z - pb.z;
        d = pa.x - pc.x;
        e = pa.y - pc.y;
        f = pa.z - pc.z;
        const auto& g = localRay[1].x;
        const auto& h = localRay[1].y;
        const auto& i = localRay[1].z;
        j = pa.x - localRay[0].x;
        k = pa.y - localRay[0].y;
        l = pa.z - localRay[0].z;

        ei_hf = (e * i) - (h * f);
        gf_di = (g * f) - (d * i);
        dh_eg = (d * h) - (e * g);
        ak_jb = (a * k) - (j * b);
        jc_al = (j * c) - (a * l);
        bl_kc = (b * l) - (k * c);

        // glm::vec3 c1 = {a, b, c};
        // glm::vec3 c2 = {d, e, f};
        // glm::vec3 c3 = {g, h, i};
        // glm::vec3 c4 = {j, k, l};

        // glm::mat3 A(c1, c2, c3);
        // auto M_ = glm::determinant(A);
        // glm::mat3 BETA(c4, c2, c3);
        // auto beta_ = glm::determinant(BETA) / M_;
        // glm::mat3 GAMMA(c1, c4, c3);
        // auto gamma_ = glm::determinant(GAMMA) / M_;
        // glm::mat3 T(c1, c2, c4);
        // auto t_ = glm::determinant(T) / M_;

        // auto t_test = t_ < t0 || t_ > tMin;
        // auto gamma_test = gamma_ < 0 || gamma_ > 1;
        // auto beta_test = beta_ < 0 || beta_ > (1 - gamma_);

        M = (a * ei_hf) + (b * gf_di) + (c * dh_eg);
        // assert(glm::epsilonEqual(M, M_, glm::epsilon<float>()));

        t = ((f * ak_jb) + (e * jc_al) + (d * bl_kc)) / -M;
        // assert(glm::epsilonEqual(t, t_, glm::epsilon<float>()));
        // test t
        if (t < t0 || t > tMin) continue;

        gamma = ((i * ak_jb) + (h * jc_al) + (g * bl_kc)) / M;
        // assert(glm::epsilonEqual(gamma, gamma_, glm::epsilon<float>()));
        // test gamma
        if (gamma < 0 || gamma > 1) continue;

        beta = ((j * ei_hf) + (k * gf_di) + (l * dh_eg)) / M;
        // assert(glm::epsilonEqual(beta, beta_, glm::epsilon<float>()));
        // test beta
        if (beta < 0 || beta > (1 - gamma)) continue;

        tMin = t;

        // TODO: below could be cleaner...
        hit = true;
        idx0_hit = idx0;
        idx1_hit = idx1;
        idx2_hit = idx2;
    }

    if (hit) {
        auto v0 = getVertexComplete(idx0_hit);
        auto v1 = getVertexComplete(idx1_hit);
        auto v2 = getVertexComplete(idx2_hit);
        v0.position = getWorldSpacePosition(v0.position);
        v1.position = getWorldSpacePosition(v1.position);
        v2.position = getWorldSpacePosition(v2.position);
        face = {v0, v1, v2, idx0_hit, idx1_hit, idx2_hit, offset};
    }
}

void Mesh::Base::updateTangentSpaceData() {
    // currently not used
    for (size_t i = 0; i < getFaceCount(); i++) {
        auto face = getFace(i);
        face.calculateTangentSpaceVectors();
        addVertex(face);
    }
}

bool Mesh::Base::shouldDraw(const PASS& passTypeComp, const PIPELINE& pipelineType) const {
    if (getInstanceCount() == 0) return false;
    if (pipelineType != PIPELINE_TYPE) return false;
    if (status_ != STATUS::READY) return false;
    for (const auto& passType : PASS_TYPES)
        if (std::visit(Pass::IsAll{}, passType) || passType == passTypeComp) return true;
    return false;
}

void Mesh::Base::draw(const RENDER_PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const vk::CommandBuffer& cmd, const uint8_t frameIndex) const {
    draw(passType, pPipelineBindData, getDescriptorSetBindData(passType), cmd, frameIndex);
}

void Mesh::Base::draw(const RENDER_PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                      const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    // bindPushConstants(cmd);

    cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                           descSetBindData.descriptorSets[setIndex], descSetBindData.dynamicOffsets);

    // VERTEX
    cmd.bindVertexBuffers(Vertex::BINDING, {vertexRes_.buffer}, {0});

    // INSTANCE (as of now there will always be at least one instance binding)
    cmd.bindVertexBuffers(          //
        getInstanceFirstBinding(),  // uint32_t firstBinding
        getInstanceBindingCount(),  // uint32_t bindingCount
        getInstanceBuffers(),       // const vk::Buffer* pBuffers
        getInstanceOffsets()        // const vk::DeviceSize* pOffsets
    );

    // TODO: clean these up!!
    if (pPipelineBindData->usesAdjacency) {
        assert(indicesAdjaceny_.size() && indexAdjacencyRes_.buffer);
        // TODO: Make index type value dynamic.
        cmd.bindIndexBuffer(indexAdjacencyRes_.buffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(                                     //
            static_cast<uint32_t>(indicesAdjaceny_.size()),  // uint32_t indexCount
            getInstanceCount(),                              // uint32_t instanceCount
            0,                                               // uint32_t firstIndex
            0,                                               // int32_t vertexOffset
            getInstanceFirstInstance()                       // uint32_t firstInstance
        );
    } else if (indices_.size()) {
        // TODO: Make index type value dynamic.
        cmd.bindIndexBuffer(indexRes_.buffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(                //
            getIndexCount(),            // uint32_t indexCount
            getInstanceCount(),         // uint32_t instanceCount
            0,                          // uint32_t firstIndex
            0,                          // int32_t vertexOffset
            getInstanceFirstInstance()  // uint32_t firstInstance
        );
    } else {
        cmd.draw(                       //
            getVertexCount(),           // uint32_t vertexCount
            getInstanceCount(),         // uint32_t instanceCount
            0,                          // uint32_t firstVertex
            getInstanceFirstInstance()  // uint32_t firstInstance
        );
    }
}

void Mesh::Base::destroy() {
    const auto& ctx = handler().shell().context();
    ctx.destroyBuffer(vertexRes_);
    ctx.destroyBuffer(indexRes_);
}

// COLOR

Mesh::Color::Color(Mesh::Handler& handler, const index&& offset, const GenericCreateInfo* pCreateInfo,
                   std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Base{
          handler,
          std::forward<const index>(offset),
          std::forward<const MESH>(MESH::COLOR),
          VERTEX::COLOR,
          std::forward<const std::string>(pCreateInfo->name),
          pCreateInfo,
          pInstanceData,
          pMaterial,
      } {
    assert(vertices_.empty() && pCreateInfo->faces.size());
    if (pCreateInfo->settings.geometryInfo.smoothNormals) {
        unique_vertices_map_smoothing vertexMap = {};
        for (auto& face : pCreateInfo->faces) const_cast<Face&>(face).indexVertices(vertexMap, this);
    } else {
        unique_vertices_map_non_smoothing vertexMap = {};
        for (auto& face : pCreateInfo->faces) const_cast<Face&>(face).indexVertices(vertexMap, this);
    }
    status_ = STATUS::PENDING_BUFFERS;
}

Mesh::Color::Color(Mesh::Handler& handler, const index&& offset, const std::string&& name, const CreateInfo* pCreateInfo,
                   std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial,
                   const MESH&& type)
    : Base{
          handler,
          std::forward<const index>(offset),
          std::forward<const MESH>(type),
          VERTEX::COLOR,
          std::forward<const std::string>(name),
          pCreateInfo,
          pInstanceData,
          pMaterial,
      } {}

Mesh::Color::~Color() = default;

// LINE

Mesh::Line::Line(Mesh::Handler& handler, const index&& offset, const std::string&& name, const CreateInfo* pCreateInfo,
                 std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Color{
          handler,
          std::forward<const index>(offset),
          std::forward<const std::string>(name),
          pCreateInfo,
          pInstanceData,
          pMaterial,
          MESH::LINE,
      } {}

Mesh::Line::~Line() = default;

// TEXTURE

Mesh::Texture::Texture(Mesh::Handler& handler, const index&& offset, const std::string&& name, const CreateInfo* pCreateInfo,
                       std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Base{
          handler,
          std::forward<const index>(offset),
          MESH::TEXTURE,
          VERTEX::TEXTURE,
          std::forward<const std::string>(name),
          pCreateInfo,
          pInstanceData,
          pMaterial,
      } {
    // This is for the, hopefully, singular mesh that is a VERTEX::SCREEN_QUAD.
    if (PIPELINE_TYPE == PIPELINE{GRAPHICS::ALL_ENUM} && PASS_TYPES.empty())
        assert(!pMaterial_->hasTexture());
    else
        assert(pMaterial_->hasTexture());
}

Mesh::Texture::~Texture() = default;
