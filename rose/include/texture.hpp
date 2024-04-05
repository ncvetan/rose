#ifndef ROSE_INCLUDE_TEXTURE
#define ROSE_INCLUDE_TEXTURE

#include <optional>
#include <string>

namespace rose {

class Texture {
  public:
    bool init(const std::string& path);

    uint32_t id;
    enum class Type { NONE, DIFFUSE, SPECULAR } type = Type::NONE;
};

std::optional<unsigned int> load_texture(const std::string& path);

} // namespace rose

#endif