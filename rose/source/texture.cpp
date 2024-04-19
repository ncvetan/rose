#include <logger.hpp>
#include <texture.hpp>

#include <GL/glew.h>
#include <stb_image.h>

#include <filesystem>
#include <format>

namespace rose {

void globals::clear_textures() {
    for (auto& [path, texture] : globals::loaded_textures) {
        glDeleteTextures(1, &texture.id);
    }
}

std::optional<TextureGL> load_texture(const std::filesystem::path& path) {
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    int width, height, n_channels;
    unsigned char* texture_data = stbi_load(path.generic_string().c_str(), &width, &height, &n_channels, 0);

    if (texture_data) {
        GLenum fmt;
        switch (n_channels) {
        case 1:
            fmt = GL_RED;
            break;
        case 2:
            fmt = GL_RG;
            break;
        case 3:
            fmt = GL_RGB;
            break;
        case 4:
            fmt = GL_RGBA;
            break;
        default:
            assert(false);
            break;
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, fmt, GL_UNSIGNED_BYTE, texture_data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(texture_data);
    } else {
        LOG_ERROR("Failed to load texture");
        stbi_image_free(texture_data);
        return std::nullopt;
    }
    TextureGL ret;
    ret.id = texture;
    ret.path = path;
    return ret;
}

TextureGL generate_texture(int w, int h) {
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    TextureGL ret;
    ret.id = texture;
    ret.type = TextureGL::Type::INTERNAL;
    return ret;
};

} // namespace rose