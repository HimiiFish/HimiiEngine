#pragma once
#include "Himii/Core/Core.h"

namespace Himii {

    struct FramebufferSpecification {
        uint32_t Width = 1;
        uint32_t Height = 1;
    };

    class Framebuffer {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;
        virtual uint32_t GetColorAttachmentRendererID() const = 0; // GL texture id

        virtual const FramebufferSpecification& GetSpecification() const = 0;

        static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
    };
}
