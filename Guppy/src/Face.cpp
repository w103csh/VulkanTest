
#include "Face.h"
#include "Mesh.h"

#include <glm/glm.hpp>
#include <glm/gtx/projection.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp>

void Face::calculateNormal() {
    glm::vec3 normal = glm::triangleNormal(vertices_[0].position, vertices_[1].position, vertices_[2].position);
    for (auto &v : vertices_) v.normal = normal;
}

void Face::calculateTangentSpaceVectors() {
    glm::vec3 t = {}, b = {};
    /*  Create a linear map from tangent space to model space. This is done using
        Cramer's rule.

        Solve:
            deltaPos1 = deltaUV1.x * T + deltaUV1.y * B
            deltaPos2 = deltaUV2.x * T + deltaUV2.y * B

            ...where T is the tangent, and B is the bitangent.

        Cramer's rule:
                | deltaPos1 deltaUV1.y  |      | deltaUV1.x deltaPos1  |
                | deltaPos2 deltaUV2.y  |      | deltaUV2.x deltaPos2  |
            T=  _________________________  B=  _________________________
                | deltaUV1.x deltaUV1.y |      | deltaUV1.x deltaUV1.y |
                | deltaUV2.x deltaUV2.y |      | deltaUV2.x deltaUV2.y |
    */
    auto deltaPos1 = vertices_[1].position - vertices_[0].position;
    auto deltaPos2 = vertices_[2].position - vertices_[0].position;

    auto deltaUV1 = vertices_[1].texCoord - vertices_[0].texCoord;
    auto deltaUV2 = vertices_[2].texCoord - vertices_[0].texCoord;

    auto denom = (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);  // divisor

    t = (deltaPos1 * deltaUV2.y - deltaUV1.y * deltaPos2) / denom;  // T
    b = (deltaUV1.x * deltaPos2 - deltaPos1 * deltaUV2.x) / denom;  // B

    // Make orthogonal (I think that doing both makes sense...
    // I should probably add some kind of test.)
    auto proj_t_onto_n = glm::proj(t, vertices_[0].normal);
    t = glm::normalize(t - proj_t_onto_n);
    auto proj_b_onto_n = glm::proj(b, vertices_[0].normal);
    b = glm::normalize(b - proj_b_onto_n);

    // Make right-handed
    if (glm::dot(glm::cross(vertices_[0].normal, t), b) < 0.0f) {
        t *= -1.0f;
    }

    for (auto &v : vertices_) {
        v.tangent = t;
        v.binormal = b;
    }
}

void Face::indexVertices(unique_vertices_map &uniqueVertices, Mesh *pMesh, bool calcTangentSpace) {
    std::vector<Mesh *> pMeshes = {pMesh};
    indexVertices(uniqueVertices, pMeshes, 0, calcTangentSpace);
}

void Face::calculateCentroid() {
    centroid_.x = (vertices_[0].position.x + vertices_[1].position.x + vertices_[2].position.x) / 3;
    centroid_.y = (vertices_[0].position.y + vertices_[1].position.y + vertices_[2].position.y) / 3;
    centroid_.z = (vertices_[0].position.z + vertices_[1].position.z + vertices_[2].position.z) / 3;
}

// Returns <0 if "this" is closer to "e" than "other"
int Face::compareCentroids(const glm::vec3 &e, Face &other) {
    calculateCentroid();
    other.calculateCentroid();

    auto dist = glm::distance(centroid_, e);
    auto distOther = glm::distance(other.centroid_, e);
    if (dist == distOther) {
        return 0;
    }
    return dist < distOther ? -1 : 1;
}