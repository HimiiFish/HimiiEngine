#pragma once
#include <string>
#include <array>
#include <vector>
#include "glm/vec2.hpp"
#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"

namespace Himii
{
    enum class ImageFormat {
        None = 0,
        R8,
        RGB8,
        RGBA8,
        RGBA32F,
        RGB16F,
        RG16F
    };

    struct TextureSpecification {
        uint32_t Width = 1;
        uint32_t Height = 1;
        ImageFormat Format = ImageFormat::RGBA8;
        bool GenerateMips = false;
        bool ClampToEdge = false;
        bool UseLinearFiltering = true;
    };

    class Texture : public Asset {
    public:
        virtual ~Texture() = default;

        virtual const TextureSpecification &GetSpecification() const = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetRendererID() const = 0;

        virtual const std::string &GetPath() const = 0;

        virtual void SetData(void *data, uint32_t size) = 0;
        virtual void SetDataRegion(
                uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, void *data,
                uint32_t size) = 0;

        virtual void Bind(uint32_t slot = 0) const = 0;

        virtual bool IsLoaded() const = 0;

        virtual bool operator==(const Texture &other) const = 0;

        virtual AssetType GetType() const override
        {
            return AssetType::Texture2D;
        }
    };

    class Texture2D : public Texture {
    public:
        static Ref<Texture2D> Create(uint32_t width, uint32_t height);
        static Ref<Texture2D> Create(const TextureSpecification &specification);
        static Ref<Texture2D> Create(const std::string &path);
    };

    class TextureCube : public Texture {
    public:
        static Ref<TextureCube> Create(const std::vector<std::string> &path);
        static Ref<TextureCube> Create(const TextureSpecification &specification);

        /// faceIndex: 0=+X … 5=-Z；data 为按 Format 紧密打包的一面像素。
        virtual void SetFaceData(uint32_t faceIndex, uint32_t mipLevel, const void *data, uint32_t size) = 0;
        virtual uint32_t GetMipLevelCount() const = 0;
    };
}; // namespace Himii
