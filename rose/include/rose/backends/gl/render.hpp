// =============================================================================
//   OpenGL renderer
// =============================================================================

#ifndef ROSE_INCLUDE_BACKENDS_GL_RENDER
#define ROSE_INCLUDE_BACKENDS_GL_RENDER

#include <rose/model.hpp>
#include <rose/backends/gl/shader.hpp>
#include <rose/backends/gl/structs.hpp>
#include <rose/core/core.hpp>

#include <glm.hpp>

#include <vector>

namespace gl {

void render(Shader& shader, const Model& model);

// this variation only renders meshes that meet a flag used as criteria (e.g., transparent meshes in a model that has a
// mix of transparent and non-transparent meshes
//
// TODO: There is probably a better way of doing this as this is only used currently to determine whether to forward
// render or defer render a mesh. Could be better to batch like-meshes together and render those without needed to check
// a condition.
void render(Shader& shader, const Model& model, MeshFlags test_flag, bool flag_on);

void render(Shader& shader, SkyBox& skybox, u32 vao);

} // namespace gl

#endif
