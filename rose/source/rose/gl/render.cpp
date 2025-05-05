#include <rose/gl/render.hpp>

namespace gl {

void RenderData::init(const std::vector<glm::vec3>& pos, const std::vector<glm::vec3>& norm,
                      const std::vector<glm::vec3>& tangent, const std::vector<glm::vec2>& uv,
                      const std::vector<u32>& indices) {

    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &pos_buf);
    glCreateBuffers(1, &norm_buf);
    glCreateBuffers(1, &tangent_buf);
    glCreateBuffers(1, &uv_buf);
    glCreateBuffers(1, &indices_buf);

    glNamedBufferStorage(pos_buf, pos.size() * sizeof(glm::vec3), pos.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(norm_buf, norm.size() * sizeof(glm::vec3), norm.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(tangent_buf, tangent.size() * sizeof(glm::vec3), tangent.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(uv_buf, uv.size() * sizeof(glm::vec2), uv.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(indices_buf, indices.size() * sizeof(u32), indices.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayElementBuffer(vao, indices_buf);

    glVertexArrayVertexBuffer(vao, 0, pos_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 0);

    glVertexArrayVertexBuffer(vao, 1, norm_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 1, 1);
    glEnableVertexArrayAttrib(vao, 1);

    glVertexArrayVertexBuffer(vao, 2, tangent_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 2, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 2, 2);
    glEnableVertexArrayAttrib(vao, 2);

    glVertexArrayVertexBuffer(vao, 3, uv_buf, 0, sizeof(glm::vec2));
    glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 3, 3);
    glEnableVertexArrayAttrib(vao, 3);
}

RenderData::RenderData(RenderData&& other) noexcept
{
    vao = other.vao;
    pos_buf = other.pos_buf;
    norm_buf = other.norm_buf;
    tangent_buf = other.tangent_buf;
    uv_buf = other.uv_buf;
    indices_buf = other.indices_buf;

    other.vao = 0;
    other.pos_buf = 0;
    other.norm_buf = 0;
    other.tangent_buf = 0;
    other.uv_buf = 0;
    other.indices_buf = 0;
}

RenderData& RenderData::operator=(RenderData&& other) noexcept
{ 
    if (this == &other) return *this;
    this->~RenderData();
    new (this) RenderData(std::move(other));
    return *this;
}

RenderData::~RenderData() {
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &pos_buf);
        glDeleteBuffers(1, &norm_buf);
        glDeleteBuffers(1, &tangent_buf);
        glDeleteBuffers(1, &uv_buf);
        glDeleteBuffers(1, &indices_buf);
    }
}

}