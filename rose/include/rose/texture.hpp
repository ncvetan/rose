#ifndef ROSE_INCLUDE_TEXTURE
#define ROSE_INCLUDE_TEXTURE

#include <rose/alias.hpp>
#include <rose/err.hpp>

#include <expected>
#include <filesystem>
#include <optional>
#include <unordered_map>

namespace rose {

enum class TextureType { NONE, DIFFUSE, SPECULAR, CUBE_MAP, INTERNAL };

struct TextureGL {
    u32 id = 0;
    TextureType ty = TextureType::NONE;

    inline void free() { glDeleteTextures(1, &id); }
};

struct TextureRef {
    TextureRef() = default;
    TextureRef(TextureGL* ref);
    TextureRef(const TextureRef& other);
    TextureRef(TextureRef&& other) noexcept;
    ~TextureRef();

    TextureRef& operator=(const TextureRef& other);
    TextureRef& operator=(TextureRef&& other) noexcept;
    TextureGL* operator->();

    TextureGL* ref = nullptr;
};

std::expected<TextureRef, rses> load_texture(const fs::path& path, TextureType ty);
std::optional<TextureRef> load_cubemap(const std::vector<fs::path>& paths);
std::optional<TextureRef> generate_texture(int w, int h);
std::optional<TextureRef> generate_cubemap(int w, int h);

struct TextureCtx {
    TextureGL texture;
    u64 refcnt = 0;
};

namespace globals {
    inline std::unordered_map<u32, TextureCtx> loaded_textures;
    inline std::unordered_map<fs::path, u32> textures_index;
}

} // namespace rose

#endif