#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <vector>

#include "Helpers.h"
#include "Vertex.h"

typedef std::array<glm::vec3, 6> BoundingBox;

const BoundingBox DefaultBoundingBox = {
    glm::vec3(FLT_MAX, 0.0f, 0.0f),   // xMin
    glm::vec3(-FLT_MAX, 0.0f, 0.0f),  // xMax
    glm::vec3(0.0f, FLT_MAX, 0.0f),   // yMin
    glm::vec3(0.0f, -FLT_MAX, 0.0f),  // yMax
    glm::vec3(0.0f, 0.0f, FLT_MAX),   // zMin
    glm::vec3(0.0f, 0.0f, -FLT_MAX)   // zMax
};

struct BoundingBoxMinMax {
    float xMin, xMax;
    float yMin, yMax;
    float zMin, zMax;
};

struct Object3d {
   public:
    struct DATA {
        glm::mat4 model = glm::mat4(1.0f);
    };

    Object3d(glm::mat4 model = glm::mat4(1.0f)) : model_{model}, boundingBox_(DefaultBoundingBox){};

    inline DATA getData() const { return {model_}; }

    virtual inline glm::vec3 worldToLocal(const glm::vec3& v, bool isPosition = false) const {
        glm::vec3 local = glm::inverse(model_) * glm::vec4(v, isPosition ? 1.0f : 0.0f);
        return isPosition ? local : glm::normalize(local);
    }

    template <typename T>
    void worldToLocal(T& vecs) const {
        auto inverse = glm::inverse(model_);
        for (auto& v : vecs) {
            v = inverse * v;
        }
    }

    virtual inline glm::vec3 getWorldSpaceDirection(const glm::vec3& d = FORWARD_VECTOR) const {
        glm::vec3 direction = model_ * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    virtual inline glm::vec3 getWorldSpacePosition(const glm::vec3& p = {}) const {  //
        return model_ * glm::vec4(p, 1.0f);
    }

    // I did the order this way so that the incoming tranform doesn't get scaled...
    // This could be problematic. Not sure yet.
    virtual inline void transform(const glm::mat4 t) { model_ = t * model_; }

    BoundingBoxMinMax getBoundingBoxMinMax(bool transform = true) const;

    virtual bool testBoundingBox(const Ray& ray, const float& tMin, bool useDirection = true) const;

    virtual void putOnTop(const BoundingBoxMinMax& boundingBox);

   protected:
    template <class T>
    inline void updateBoundingBox(const std::vector<T>& vs) {
        for (auto& v : vs) updateBoundingBox(v);
    }

    template <class T>
    inline void updateBoundingBox(const T& v) {
        if (v.position.x < boundingBox_[0].x) boundingBox_[0] = v.position;  // xMin
        if (v.position.x > boundingBox_[1].x) boundingBox_[1] = v.position;  // xMax
        if (v.position.y < boundingBox_[2].y) boundingBox_[2] = v.position;  // yMin
        if (v.position.y > boundingBox_[3].y) boundingBox_[3] = v.position;  // yMax
        if (v.position.z < boundingBox_[4].z) boundingBox_[4] = v.position;  // zMin
        if (v.position.z > boundingBox_[5].z) boundingBox_[5] = v.position;  // zMax
    }

    inline void updateBoundingBox(const BoundingBoxMinMax& bbmm) {
        if (bbmm.xMin < boundingBox_[0].x) boundingBox_[0].x = bbmm.xMin;  // xMin
        if (bbmm.xMax > boundingBox_[1].x) boundingBox_[1].x = bbmm.xMax;  // xMax
        if (bbmm.yMin < boundingBox_[2].y) boundingBox_[2].y = bbmm.yMin;  // yMin
        if (bbmm.yMax > boundingBox_[3].y) boundingBox_[3].y = bbmm.yMax;  // yMax
        if (bbmm.zMin < boundingBox_[4].z) boundingBox_[4].z = bbmm.zMin;  // zMin
        if (bbmm.zMax > boundingBox_[5].z) boundingBox_[5].z = bbmm.zMax;  // zMax
    }

    BoundingBox getBoundingBox() const;

    // Model space to world space
    glm::mat4 model_ = glm::mat4(1.0f);

   private:
    /* This needs to be transformed so it should be private!
       TODO: If a vertex is changed then this needs to be updated using one
       of the updateBoundingBox functions... this is not great. ATM it would
       be safer to just always use the update that takes all the vertices.
    */
    BoundingBox boundingBox_;
};

#endif  // !OBJECT3D_H