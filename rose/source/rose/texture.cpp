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

TextureGL* TextureRef::operator->() {
    return this->ref;
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
            // stale index
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

std::expected<TextureRef, rses> load_texture(const fs::path& path, TextureType ty) {

    // First check to see if the texture has already been loaded
    std::optional<TextureRef> opt_ref = get_texture_ref(path);
    if (opt_ref) {
        return opt_ref.value();
    }

    TextureGL texture;
    texture.ty = ty;
    i32 width = 0, height = 0, n_channels = 0;
    unsigned char* texture_data = stbi_load(path.generic_string().c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

    if (texture_data) {
        glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);
        glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(texture.id, 1, GL_RGBA8, width, height);
        glTextureSubImage2D(texture.id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
        glGenerateTextureMipmap(texture.id);
        stbi_image_free(texture_data);
    } else {
        LOG_ERROR("Failed to load texture");
        stbi_image_free(texture_data);
        const char* err_msg = stbi_failure_reason();
        texture.free();
        return std::unexpected(rses().io("failed to load image: {}", err_msg));
    }

    globals::loaded_textures[texture.id] = { texture, 1 };
    globals::textures_index[path] = texture.id;
    return TextureRef(&globals::loaded_textures[texture.id].texture);
}

std::optional<TextureRef> load_cubemap(const std::vector<fs::path>& paths) {
    
    if (paths.size() != 6) {
        return std::nullopt; // todo: improve err handling here. arg can be a std::array
    }

    i32 width = 0, height = 0, n_channels = 0;
    unsigned char* texture_data = nullptr;
    TextureGL texture;
    texture.ty = TextureType::CUBE_MAP;

    for (int face = 0; face < 6; face++) {
        
        const fs::path& path = paths[face];
        texture_data = stbi_load(path.generic_string().c_str(), &width, &height, &n_channels, STBI_rgb_alpha);
        
        // initialize the cube map based off the first texture in the list
        if (texture.id == 0) {
            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture.id);
            glTextureStorage2D(texture.id, 1, GL_RGBA8, width, height);
        }
        
        if (!texture_data) {
            stbi_image_free(texture_data);
            texture.free();
            return std::nullopt;
        }

        fs::path name = path.stem();

        if ((face == 0 && name != "right")  || 
            (face == 1 && name != "left")   ||
            (face == 2 && name != "top")    || 
            (face == 3 && name != "bottom") || 
            (face == 4 && name != "front")  || 
            (face == 5 && name != "back")) {
            stbi_image_free(texture_data);
            texture.free();
            return std::nullopt;
        }

        glTextureSubImage3D(texture.id, 0, 0, 0, face, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
        stbi_image_free(texture_data);
    }

    glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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

std::optional<TextureRef> generate_cubemap(int w, int h) {
    TextureGL cubemap = { 0, TextureType::CUBE_MAP };
    glGenTextures(1, &cubemap.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);

    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    globals::loaded_textures[cubemap.id] = { cubemap, 1 };
    return TextureRef(&globals::loaded_textures[cubemap.id].texture);
};

} // namespace rose