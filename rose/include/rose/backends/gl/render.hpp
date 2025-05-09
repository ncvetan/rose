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

// renders only the opaque meshes of a model
void render_opaque(Shader& shader, const Model& model);

// renders only the transparent meshes of a model
void render_transparent(Shader& shader, const Model& model);

void render(Shader& shader, SkyBox& skybox, u32 vao);

} // namespace gl

#endif
