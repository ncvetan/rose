// =============================================================================
//   OpenGL renderer
// =============================================================================

#include <rose/model.hpp>
#include <rose/core/core.hpp>

#include <glm.hpp>

#include <vector>

#ifndef ROSE_INCLUDE_GL_RENDER
#define ROSE_INCLUDE_GL_RENDER

namespace gl {

struct RenderData {

    void init(const std::vector<glm::vec3>& pos, const std::vector<glm::vec3>& norm,
              const std::vector<glm::vec3>& tangent, const std::vector<glm::vec2>& uv, const std::vector<u32>& indices);

    ~RenderData();

    u32 vao = 0;
    u32 pos_buf = 0;
    u32 norm_buf = 0;
    u32 tangent_buf = 0;
    u32 uv_buf = 0;
    u32 indices_buf = 0;
};

} // namespace gl

#endif
