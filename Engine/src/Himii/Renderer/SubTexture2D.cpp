#include "Hepch.h"
#include "Himii/Renderer/SubTexture2D.h"

namespace Himii
{
    SubTexture2D::SubTexture2D(const Ref<Texture2D> &texture, const glm::vec2 &min, const glm::vec2 &max) :
        m_Texture(texture)
    {
        m_TexCoords[0] = {min.x, min.y};
    }
} // namespace Himii
