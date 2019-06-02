/*
    Parallax Shader

    This comes from the cookbook.
*/

#ifndef PARALLAX_H
#define PARALLAX_H

#include "Constants.h"
#include "DescriptorSet.h"
#include "Pipeline.h"

namespace Descriptor {
namespace Set {
namespace Parallax {
class Uniform : public Set::Base {
   public:
    Uniform();
};
class Sampler : public Set::Base {
   public:
    Sampler();
};
}  // namespace Parallax
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
namespace Parallax {
extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo SIMPLE_CREATE_INFO;
extern const CreateInfo STEEP_CREATE_INFO;
}  // namespace Parallax
}  // namespace Shader

namespace Pipeline {
struct CreateInfoResources;
namespace Parallax {
class Simple : public Pipeline::Base {
   public:
    Simple(Pipeline::Handler &handler);
};
class Steep : public Pipeline::Base {
   public:
    Steep(Pipeline::Handler &handler);
};
}  // namespace Parallax
}  // namespace Pipeline

#endif  // !PARALLAX_H