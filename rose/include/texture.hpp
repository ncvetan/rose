#ifndef ROSE_INCLUDE_TEXTURE
#define ROSE_INCLUDE_TEXTURE

#include <optional>
#include <string>
#include <filesystem>

namespace rose {

struct Texture {
    uint32_t id;
    enum class Type { NONE, DIFFUSE, SPECULAR } type = Type::NONE;
    std::filesystem::path path;
};

std::optional<unsigned int> load_texture(const std::filesystem::path& path);

} // namespace rose

#endif