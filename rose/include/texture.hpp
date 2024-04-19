#ifndef ROSE_INCLUDE_TEXTURE
#define ROSE_INCLUDE_TEXTURE

#include <filesystem>
#include <optional>
#include <string>

#include <alias.hpp>

namespace rose {

struct TextureGL {
    u32 id = 0;
    enum class Type { NONE, DIFFUSE, SPECULAR, INTERNAL } type = Type::NONE;
    std::filesystem::path path;
};

std::optional<TextureGL> load_texture(const std::filesystem::path& path);
TextureGL generate_texture(int w, int h);

namespace globals {
    inline std::unordered_map<std::string, TextureGL> loaded_textures;
    void clear_textures();
}

} // namespace rose

#endif