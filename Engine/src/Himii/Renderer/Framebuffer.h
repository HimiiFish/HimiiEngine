#pragma once
#include "Himii/Core/Core.h"

namespace Himii {

    struct FramebufferSpecification {
        uint32_t Width = 1;
        uint32_t Height = 1;
    // Optional: enable entity picking (R32UI color attachment + depth)
    bool EnablePicking = false;
    };

    class Framebuffer {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;
        virtual uint32_t GetColorAttachmentRendererID() const = 0; // GL texture id
    // If picking is enabled, returns GL texture id of R32UI attachment; otherwise 0
    virtual uint32_t GetPickingAttachmentRendererID() const { return 0; }
    // Read one pixel from picking attachment (x,y), return uint32 id; if unsupported, returns 0
    virtual uint32_t ReadPickingPixel(uint32_t /*x*/, uint32_t /*y*/) const { return 0; }

        virtual const FramebufferSpecification& GetSpecification() const = 0;

        static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
    };
}
