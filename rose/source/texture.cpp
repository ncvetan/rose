#include <logger.hpp>
#include <texture.hpp>

#include <GL/glew.h>
#include <stb_image.h>

#include <format>

namespace rose {

std::optional<unsigned int> load_texture(const std::string& path) {

    unsigned int texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, n_channels;
    unsigned char* texture_data = stbi_load(path.c_str(), &width, &height, &n_channels, 3);

    if (texture_data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        LOG_ERROR("Failed to load texture");
        return std::nullopt;
    }

    stbi_image_free(texture_data);

    return texture;
}
} // namespace rose