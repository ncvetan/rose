#include <rose/logger.hpp>
#include <rose/texture.hpp>

#include <GL/glew.h>
#include <stb_image.h>

#include <filesystem>

namespace rose {

TextureRef::TextureRef(TextureGL* ref) : ref(ref) {}

TextureRef::TextureRef(const TextureRef& other) {
    ref = other.ref;
    globals::loaded_textures[ref->id].refcnt++;
}

TextureRef& TextureRef::operator=(const TextureRef& other) {
    if (this == &other) return *this;
    this->~TextureRef();
    new (this) TextureRef(other);
    return *this;
}

TextureRef::TextureRef(TextureRef&& other) noexcept {
    ref = other.ref;
    other.ref = nullptr;
}

TextureRef& TextureRef::operator=(TextureRef&& other) noexcept {
    if (this == &other) return *this;
    this->~TextureRef();
    new (this) TextureRef(std::move(other));
    return *this;
}

TextureRef::~TextureRef() {
    if (ref && globals::loaded_textures.contains(ref->id)) {
        auto& [texture, refcnt] = globals::loaded_textures[ref->id];
        refcnt -= 1;
        if (refcnt == 0) {
            texture.free();
            globals::loaded_textures.erase(ref->id);
        }
        ref = nullptr;
    }
}

// if the texture at the given path has been loaded, return a reference to it
static std::optional<TextureRef> get_texture_ref(const fs::path& path) {
    if (globals::textures_index.contains(path)) {
        u32 val = globals::textures_index[path];
        if (globals::loaded_textures.contains(val)) {
            TextureRef ref;
            ref.ref = &globals::loaded_textures[val].texture;
            globals::loaded_textures[val].refcnt++;
            return ref;
        } else {
            // Stale index
            globals::textures_index.erase(path);
        }
    }
    return std::nullopt;
}

static std::optional<TextureRef> get_texture_ref(u32 id) {
    if (globals::loaded_textures.contains(id)) {
        TextureRef ref;
        ref.ref = &globals::loaded_textures[id].texture;
        globals::loaded_textures[id].refcnt++;
        return ref;
    }
    return std::nullopt;
}

std::optional<TextureRef> load_texture(const fs::path& path, TextureType ty) {

    // First check to see if the texture has already been loaded
    std::optional<TextureRef> opt_ref = get_texture_ref(path);
    if (opt_ref) {
        return opt_ref;
    }

    TextureGL texture;
    texture.ty = ty;
    glGenTextures(1, &texture.id);
    i32 width = 0, height = 0, n_channels = 0;
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
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, fmt, GL_UNSIGNED_BYTE, texture_data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(texture_data);
    } else {
        LOG_ERROR("Failed to load texture");
        stbi_image_free(texture_data);
        glDeleteTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, 0);
        return std::nullopt;
    }

    globals::loaded_textures[texture.id] = { texture, 1 };
    globals::textures_index[path] = texture.id;
    return TextureRef(&globals::loaded_textures[texture.id].texture);
}

std::optional<TextureRef> load_cubemap(const std::vector<fs::path>& paths) {
    
    if (paths.size() != 6) {
        return std::nullopt; // todo: improve err handling here. arg can really be a std::array
    }

    i32 width = 0, height = 0, n_channels = 0;
    unsigned char* texture_data = nullptr;
    TextureGL texture;
    texture.ty = TextureType::CUBE_MAP;
    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture.id);
    
    for (const auto& path : paths) {
        texture_data = stbi_load(path.generic_string().c_str(), &width, &height, &n_channels, 0);
        if (!texture_data) {
            stbi_image_free(texture_data);
            texture.free();
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            return std::nullopt;
        }
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

        GLenum target = 0;
        fs::path name = path.stem();

        if (name == "right") {
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        } else if (name == "left") {
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        } else if (name == "top") {
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        } else if (name == "bottom") {
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        } else if (name == "front") {
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        } else if (name == "back") {
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        } else {
            stbi_image_free(texture_data);
            texture.free();
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            return std::nullopt;
        }

        glTexImage2D(target, 0, GL_RGB, width, height, 0, fmt, GL_UNSIGNED_BYTE,
                     texture_data);
        stbi_image_free(texture_data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    globals::loaded_textures[texture.id] = { texture, 1 };
    return TextureRef(&globals::loaded_textures[texture.id].texture);
}

std::optional<TextureRef> generate_texture(int w, int h) {
    TextureGL texture = { 0, TextureType::INTERNAL };
    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    globals::loaded_textures[texture.id] = { texture, 1 };
    return TextureRef(&globals::loaded_textures[texture.id].texture);
};

} // namespace rose