#ifndef ROSE_INCLUDE_TEXTURE
#define ROSE_INCLUDE_TEXTURE

#include <filesystem>
#include <optional>
#include <string>

#include <alias.hpp>

namespace rose {

enum class TextureType { NONE, DIFFUSE, SPECULAR, CUBE_MAP, INTERNAL };

struct TextureGL {
    u32 id = 0;
    TextureType ty = TextureType::NONE;

    inline void free() { glDeleteTextures(1, &id); }
};

struct TextureRef {
    TextureRef() = default;
    TextureRef(u32 id, TextureGL* ref);
    TextureRef(const TextureRef& other);
    TextureRef(TextureRef&& other) noexcept;
    ~TextureRef();

    TextureRef& operator=(const TextureRef& other);
    TextureRef& operator=(TextureRef&& other) noexcept;

    u32 id = 0;
    TextureGL* ref = nullptr;
};

std::optional<TextureRef> load_texture(const fs::path& path, TextureType ty);
std::optional<TextureRef> load_cubemap(const std::vector<fs::path>& paths);
std::optional<TextureRef> generate_texture(int w, int h);

namespace globals {
    inline std::unordered_map<u32, std::pair<TextureGL, u64>> loaded_textures;
    inline std::unordered_map<fs::path, u32> textures_index;
}

} // namespace rose

#endif