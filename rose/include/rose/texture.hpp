// =============================================================================
//   structures for managing textures
// =============================================================================

#ifndef ROSE_INCLUDE_TEXTURE
#define ROSE_INCLUDE_TEXTURE

#include <rose/core/core.hpp>
#include <rose/core/err.hpp>

#include <array>
#include <expected>
#include <filesystem>
#include <optional>
#include <unordered_map>

enum class TextureType { 
    NONE = 0, 
    DIFFUSE, 
    ALBEDO, 
    SPECULAR, 
    NORMAL, 
    DISPLACE,
    GLTF_PBR,
    ROUGHNESS,
    METALLIC,
    AMBIENT_OCCLUSION,
    CUBE_MAP, 
    INTERNAL,
    TEXTURE_COUNT
};

enum class TextureFormat { 
    NONE = 0,
    RGBA8,
    RGBA16F,
    FORMAT_COUNT
};

enum class TextureFlags : u32 {
    NONE = 0,            // no effect
    TRANSPARENT = bit1   // this texture has some degree of transparency
};

ENABLE_ROSE_ENUM_OPS(TextureFlags);

struct GL_Texture {
    u32 id = 0;
    TextureType ty = TextureType::NONE;
    TextureFlags flags = TextureFlags::NONE;

    inline void set_min_filter(i32 param) {
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, param);
    }

    inline void set_mag_filter(i32 param) {
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, param);
    }

    inline void set_wrap(i32 param) {
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, param);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, param);    
    }

    inline void free() { glDeleteTextures(1, &id); }
};

struct TextureCount {
    GL_Texture texture;
    u64 ref_count = 0;
};

struct TextureManager;

// reference to a texture, will reduce reference count on destruction
struct TextureRef {
    TextureRef() = default;
    TextureRef(GL_Texture* ref, TextureManager* manager);
    TextureRef(const TextureRef& other);        // increases ref count
    TextureRef(TextureRef&& other) noexcept;    // does not increase ref count
    ~TextureRef();

    TextureRef& operator=(const TextureRef& other);
    TextureRef& operator=(TextureRef&& other) noexcept;
    GL_Texture* operator->();

    GL_Texture* ref = nullptr;
    TextureManager* manager = nullptr;
};

// This struct manages textures within the program. It can provide references to textures that will be used
// perform shared memory management.
struct TextureManager {

    void init();
    
    TextureRef load_texture(const fs::path& path, TextureType ty);
    TextureRef load_cubemap(const std::array<fs::path, 6>& paths);

    TextureRef gen_texture(u32 w, u32 h, TextureFormat format);

    TextureRef get_ref(const fs::path& path);
    TextureRef get_ref(u32 id);

    std::unordered_map<u32, TextureCount> loaded_textures;  // [ id,  tex ]
    std::unordered_map<fs::path, u32> textures_index;       // [ path, id ]

    // references to default textures, returned in cases of errors
    TextureRef default_tex_ref;
    TextureRef default_cubemap_ref;
};


#endif