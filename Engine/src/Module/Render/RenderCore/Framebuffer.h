#pragma once
#include "EngineCore/Core/Core.h"
#include <cstdint>
#include <vector>

namespace Himii
{
    enum class FramebufferFormat
    {
        None = 0,

        //Color
        RGBA8,
        RED_INTEGER,
        
        DEPTH24STENCIL8,
        /// 仅深度（阴影贴图）：支持 sampler2DShadow 硬件比较。
        DEPTH32,

        // Defaults
        Depth = DEPTH24STENCIL8
    };

    struct FramebufferTextureSpecification {

        FramebufferTextureSpecification() = default;
        FramebufferTextureSpecification(FramebufferFormat format) : TextureFormat(format)
        {
        }

        FramebufferFormat TextureFormat = FramebufferFormat::None;
    };

    struct FramebufferAttachmentSpecification {

        FramebufferAttachmentSpecification() = default;
        FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments) :
            Attachments(attachments)
        {
        }

        std::vector<FramebufferTextureSpecification> Attachments;
    };

    struct FramebufferSpecification
    {
        uint32_t Width = 0;
        uint32_t Height = 0;

        FramebufferAttachmentSpecification Attachments;
        uint32_t Samples = 1;

        bool SwapChainTarget = false;
    };

    class Framebuffer {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;

        /// 绑定前捕获当前 FBO 与 viewport，供阴影 pass 结束后还原场景目标。
        virtual void BindCapturingPrevious() = 0;
        virtual void UnbindRestoringPrevious() = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;
        virtual int ReadPixel(uint32_t attachmentIndex, int x ,int y) = 0;

        virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

        /// 读取颜色附件 RGBA8 像素（行自下而上，与 OpenGL 默认一致）。
        virtual void ReadColorPixels(uint32_t attachmentIndex, std::vector<uint8_t> &outRgbaBytes) = 0;

        virtual uint32_t GetColorAttachmentRendererID(uint32_t index=0) const = 0;
        virtual uint32_t GetDepthAttachmentRendererID() const = 0;
        virtual void BindDepthAttachment(uint32_t slot) const = 0;

        virtual const FramebufferSpecification &GetSpecification() const = 0;

        static Ref<Framebuffer> Create(const FramebufferSpecification &spec);
    };
} // namespace Himii
