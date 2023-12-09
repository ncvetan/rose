#include <logger.hpp>
#include <texture.hpp>

#include <GL/glew.h>
#include <stb_image.h>

#include <format>

namespace rose {

std::optional<unsigned int> load_texture(const std::string& path) {

    unsigned int texture;

    glGenTextures(1, &texture);

    int width, height, n_channels;
    unsigned char* texture_data = stbi_load(path.c_str(), &width, &height, &n_channels, 0);

    if (texture_data) {
        GLenum fmt;
        switch (n_channels) {
        case 1:
            fmt = GL_RED;
            break;
        case 3:
            fmt = GL_RGB;
            break;
        case 4:
            [[fallthrough]];
        default:
            fmt = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, fmt, GL_UNSIGNED_BYTE, texture_data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(texture_data);
    } else {
        LOG_ERROR("Failed to load texture");
        stbi_image_free(texture_data);
        return std::nullopt;
    }

    return texture;
}
} // namespace rose