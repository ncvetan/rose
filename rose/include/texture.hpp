#ifndef ROSE_INCLUDE_TEXTURE
#define ROSE_INCLUDE_TEXTURE

#include <optional>
#include <string>

namespace rose {

class Texture {
  public:
    bool init(const std::string& path){};
};

std::optional<unsigned int> load_texture(const std::string& path);

} // namespace rose

#endif