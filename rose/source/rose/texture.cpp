#include <rose/texture.hpp>

#include <GL/glew.h>
#include <stb_image.h>

#include <filesystem>

TextureRef::TextureRef(GL_Texture* ref, TextureManager* manager) : ref(ref), manager(manager) {}

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

GL_Texture* TextureRef::operator->() {
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

TextureRef TextureManager::get_ref(const fs::path& path) {
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
    return default_tex_ref;
}

TextureRef TextureManager::get_ref(u32 id) {
    if (loaded_textures.contains(id)) {
        TextureRef ref = TextureRef(&loaded_textures[id].texture, this);
        loaded_textures[id].ref_count++;
        return ref;
    }
    return default_tex_ref;
}

void TextureManager::init() {
    // creating a default texture for errors
    GL_Texture default_texture;
    default_texture.ty = TextureType::DIFFUSE;
    std::vector<glm::uvec4> default_data(1024 * 1024, glm::uvec4(255, 0, 0, 255));
    
    glCreateTextures(GL_TEXTURE_2D, 1, &default_texture.id);
    glTextureParameteri(default_texture.id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(default_texture.id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(default_texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(default_texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(default_texture.id, 1, GL_RGBA8, 1024, 1024);
    glTextureSubImage2D(default_texture.id, 0, 0, 0, 1024, 1024, GL_RGBA, GL_UNSIGNED_BYTE, default_data.data());

    GL_Texture default_cubemap;
    default_cubemap.ty = TextureType::CUBE_MAP;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &default_cubemap.id);
    glTextureStorage2D(default_cubemap.id, 1, GL_RGBA8, 1024, 1024);

    for (int face = 0; face < 6; face++) {
        glTextureSubImage3D(default_cubemap.id, 0, 0, 0, face, 1024, 1024, 1, GL_RGBA, GL_UNSIGNED_BYTE, default_data.data());
    }

    glTextureParameteri(default_cubemap.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(default_cubemap.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(default_cubemap.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(default_cubemap.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(default_cubemap.id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // note: don't really care about the ref count here since it will free on program termination 
    loaded_textures[default_texture.id] = { default_texture, 1024 };
    loaded_textures[default_cubemap.id] = { default_cubemap, 1024 };
    default_tex_ref = TextureRef(&loaded_textures[default_texture.id].texture, this);
    default_cubemap_ref = TextureRef(&loaded_textures[default_cubemap.id].texture, this);
}

TextureRef TextureManager::load_texture(const fs::path& path, TextureType ty) {

    // First check to see if the texture has already been loaded
    TextureRef ref = get_ref(path);
    
    if (ref->id != default_tex_ref->id) {
        return ref;
    }

    GL_Texture texture;
    texture.ty = ty;
    i32 width = 0, height = 0, n_channels = 0;
    unsigned char* texture_data = stbi_load(path.generic_string().c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

    if (n_channels == 4) {
        // this texture has an alpha channel
        texture.flags = TextureFlags::TRANSPARENT;
    }

    if (texture_data) {
        i32 n_levels = 1 + (int)std::floor(std::log2((double)std::max(width, height)));
        glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);
        glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(texture.id, n_levels, GL_RGBA8, width, height);
        glTextureSubImage2D(texture.id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
        glGenerateTextureMipmap(texture.id);
        stbi_image_free(texture_data);
    } 
    else {
        stbi_image_free(texture_data);
        const char* err_msg = stbi_failure_reason();
        texture.free();
        return default_tex_ref;
    }

    loaded_textures[texture.id] = { texture, 1 };
    textures_index[path] = texture.id;
    return TextureRef(&loaded_textures[texture.id].texture, this);
}

TextureRef TextureManager::load_cubemap(const std::array<fs::path, 6>& paths) {
    
    // ensure all paths are valid
    for (auto& path : paths) {
        std::error_code err;
        bool f_exists = fs::exists(path, err);
        if (err || !f_exists) {
            return default_cubemap_ref;
        }
    }

    i32 width = 0, height = 0, n_channels = 0;
    unsigned char* texture_data = nullptr;
    GL_Texture texture = {
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
            return default_cubemap_ref;
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

TextureRef TextureManager::gen_texture(u32 width, u32 height, TextureFormat format) {
    
    GL_Texture texture;
    texture.ty = TextureType::INTERNAL;
    GLenum gl_format;

    switch (format) { 
    case TextureFormat::RGBA8:
        gl_format = GL_RGBA8;
        break;
    case TextureFormat::RGBA16F:
        gl_format = GL_RGBA16F;
        break;
    default:
        assert(false);
        return default_tex_ref;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);
    glTextureStorage2D(texture.id, 1, gl_format, width, height);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    loaded_textures[texture.id] = { texture, 1 };
    return TextureRef(&loaded_textures[texture.id].texture, this);
}
