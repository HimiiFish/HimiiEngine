#include "Hepch.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Module/Render/RHI/RHI.h"

namespace Himii
{
    Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
    {
        TextureSpecification specification;
        specification.Width = width;
        specification.Height = height;
        specification.Format = ImageFormat::RGBA8;
        return Create(specification);
    }

    Ref<Texture2D> Texture2D::Create(const TextureSpecification &specification)
    {
        return RHI::CreateTexture2D(specification);
    }

    Ref<Texture2D> Texture2D::Create(const std::string &path)
    {
        return RHI::CreateTexture2D(path);
    }

    Ref<TextureCube> TextureCube::Create(const std::vector<std::string> &paths)
    {
        return RHI::CreateTextureCube(paths);
    }

    Ref<TextureCube> TextureCube::Create(const TextureSpecification &specification)
    {
        return RHI::CreateTextureCube(specification);
    }
}
