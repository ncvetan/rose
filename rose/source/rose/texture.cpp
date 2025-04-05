#include <rose/texture.hpp>

#include <GL/glew.h>
#include <stb_image.h>

#include <filesystem>

namespace rose {

TextureRef::TextureRef(TextureGL* ref, TextureManager* manager) : ref(ref), manager(manager) {}

TextureRef::TextureRef(const TextureRef& other) {
    ref = other.ref;
    manager = other.manager;
    manager->loaded_textures[ref->id].ref_count++;
}

TextureRef& TextureRef::operator=(const TextureRef& other) {
    if (this == &other) return *this;
    this->~TextureRef();
    new (this) TextureRef(other);
    return *this;
}

TextureRef::TextureRef(TextureRef&& other) noexcept {
    ref = other.ref;
    manager = other.manager;
    other.ref = nullptr;
    other.manager = nullptr;
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
    if (ref && manager && manager->loaded_textures.contains(ref->id)) {
        auto& [texture, ref_count] = manager->loaded_textures[ref->id];
        if (ref_count > 0) {
            ref_count -= 1;
        }
        if (ref_count == 0) {
            texture.free();
            manager->loaded_textures.erase(ref->id);
        }
    }
    ref = nullptr;
    manager = nullptr;
}

std::optional<TextureRef> TextureManager::get_ref(const fs::path& path) {
    // if a path is reused as another texture type, this will not work
    
    if (textures_index.contains(path)) {
        u32 val = textures_index[path];
        if (loaded_textures.contains(val)) {
            TextureRef ref = TextureRef(&loaded_textures[val].texture, this);
            loaded_textures[val].ref_count++;
            return ref;
        } 
        else {
            // stale index
            textures_index.erase(path);
        }
    }
    return std::nullopt;
}

std::optional<TextureRef> TextureManager::get_ref(u32 id) {
    if (loaded_textures.contains(id)) {
        TextureRef ref = TextureRef(&loaded_textures[id].texture, this);
        loaded_textures[id].ref_count++;
        return ref;
    }
    return std::nullopt;
}

std::expected<TextureRef, rses> TextureManager::load_texture(const fs::path& path, TextureType ty) {

    // First check to see if the texture has already been loaded
    std::optional<TextureRef> opt_ref = get_ref(path);
    if (opt_ref) {
        return opt_ref.value();
    }

    TextureGL texture;
    texture.ty = ty;
    i32 width = 0, height = 0, n_channels = 0;
    unsigned char* texture_data = stbi_load(path.generic_string().c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

    if (n_channels == 4) {
        // this texture has an alpha channel
        texture.flags = TextureFlags::TRANSPARENT;
    }

    if (texture_data) {
        glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);
        glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(texture.id, 1, GL_RGBA8, width, height);
        glTextureSubImage2D(texture.id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
        glGenerateTextureMipmap(texture.id);
        stbi_image_free(texture_data);
    } 
    else {
        stbi_image_free(texture_data);
        const char* err_msg = stbi_failure_reason();
        texture.free();
        return std::unexpected(rses().io("failed to load image: {}", err_msg));
    }

    loaded_textures[texture.id] = { texture, 1 };
    textures_index[path] = texture.id;
    return TextureRef(&loaded_textures[texture.id].texture, this);
}

std::expected<TextureRef, rses> TextureManager::load_cubemap(const std::array<fs::path, 6>& paths) {
    
    // ensure all paths are valid
    for (auto& path : paths) {
        std::error_code err;
        bool f_exists = fs::exists(path, err);
        if (err) {
            return std::unexpected(rses().io("system error: {}", err.message()));
        }
        if (!f_exists) {
            return std::unexpected(rses().io("file does not exists: {}", path.string()));
        }
    }

    i32 width = 0, height = 0, n_channels = 0;
    unsigned char* texture_data = nullptr;
    TextureGL texture = {
        .id = 0,
        .ty = TextureType::CUBE_MAP
    };

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
            return std::unexpected(rses().io("failure loading image with stb"));;
        }

        glTextureSubImage3D(texture.id, 0, 0, 0, face, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
        stbi_image_free(texture_data);
    }

    glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    loaded_textures[texture.id] = { texture, 1 };
    return TextureRef(&loaded_textures[texture.id].texture, this);
}

} // namespace rose