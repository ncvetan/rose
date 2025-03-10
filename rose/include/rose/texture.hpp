#ifndef ROSE_INCLUDE_TEXTURE
#define ROSE_INCLUDE_TEXTURE

#include <rose/core/alias.hpp>
#include <rose/core/err.hpp>

#include <array>
#include <expected>
#include <filesystem>
#include <optional>
#include <unordered_map>

namespace rose {

enum class TextureType { 
    NONE = 0, 
    DIFFUSE, 
    SPECULAR, 
    NORMAL, 
    DISPLACE, 
    CUBE_MAP, 
    INTERNAL,
    TEXTURE_COUNT
};

struct TextureGL {
    u32 id = 0;
    TextureType ty = TextureType::NONE;

    inline void free() { glDeleteTextures(1, &id); }
};

struct TextureCount {
    TextureGL texture;
    u64 ref_count = 0;
};

struct TextureRef;

// This class manages textures within the program. It can provide references to textures that will be used
// perform shared memory management
struct TextureManager {
    std::expected<TextureRef, rses> load_texture(const fs::path& path, TextureType ty);
    std::expected<TextureRef, rses> load_cubemap(const std::array<fs::path, 6>& paths);

    std::optional<TextureRef> get_ref(const fs::path& path);
    std::optional<TextureRef> get_ref(u32 id);

    std::unordered_map<u32, TextureCount> loaded_textures;  // [ id,  tex ]
    std::unordered_map<fs::path, u32> textures_index;       // [ path, id ]
};

// reference to a texture, will reduce reference count on destruction
struct TextureRef {
    TextureRef() = default;
    TextureRef(TextureGL* ref, TextureManager* manager);
    TextureRef(const TextureRef& other);        // increases ref count
    TextureRef(TextureRef&& other) noexcept;    // does not increase ref count
    ~TextureRef();

    TextureRef& operator=(const TextureRef& other);
    TextureRef& operator=(TextureRef&& other) noexcept;
    TextureGL* operator->();

    TextureGL* ref = nullptr;
    TextureManager* manager = nullptr;
};

} // namespace rose

#endif