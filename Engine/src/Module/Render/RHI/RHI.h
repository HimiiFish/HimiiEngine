#pragma once
#include "EngineCore/Core/Core.h"
#include "Module/Render/RenderCore/VertexArray.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Module/Render/RenderCore/Shader.h"
#include "Module/Render/RenderCore/Framebuffer.h"
#include "Module/Render/RenderCore/UniformBuffer.h"
#include "Module/Render/RenderCore/Buffer.h"
#include "Module/Render/RenderCore/GraphicsContext.h"
#include "glm/glm.hpp"

#include <string>
#include <vector>

namespace Himii
{
    /// 渲染硬件接口：命令提交 + 资源/上下文创建工厂。上层不得直触 Platform/OpenGL。
    class RHI
    {
    public:
        enum class API
        {
            None = 0,
            OpenGL,
            Vulkan,
            DirectX12,
            Metal
        };

        enum class DepthComp
        {
            Never,
            Less,
            Equal,
            Lequal,
            Greater,
            Notequal,
            Gequal,
            Always
        };

        enum class CullMode
        {
            Front,
            Back,
            FrontAndBack,
            None
        };

        virtual ~RHI() = default;

        virtual void Init() = 0;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void SetClearColor(const glm::vec4 &color) = 0;
        virtual void Clear() = 0;

        virtual void DrawIndexed(const Ref<VertexArray> &vertexArray, uint32_t indexCount = 0) = 0;
        virtual void DrawIndexedInstanced(
                const Ref<VertexArray> &vertexArray, uint32_t indexCount, uint32_t instanceCount) = 0;
        virtual void DrawArrays(const Ref<VertexArray> &vertexArray, uint32_t vertexCount = 0) = 0;
        virtual void DrawLines(const Ref<VertexArray> &vertexArray, uint32_t indexCount = 0) = 0;
        virtual void SetLineWidth(float width) = 0;
        virtual void SetDepthTest(bool enabled) = 0;
        virtual void SetDepthMask(bool enabled) = 0;
        virtual void SetDepthFunc(DepthComp function) = 0;
        virtual void SetCullMode(CullMode mode) = 0;

        static API GetAPI() { return s_API; }
        static Scope<RHI> Create();

        static Ref<VertexBuffer> CreateVertexBuffer(uint32_t size);
        static Ref<VertexBuffer> CreateVertexBuffer(float *vertices, uint32_t size);
        static Ref<IndexBuffer> CreateIndexBuffer(uint32_t *indices, uint32_t count);
        static Ref<VertexArray> CreateVertexArray();
        static Ref<Texture2D> CreateTexture2D(const TextureSpecification &specification);
        static Ref<Texture2D> CreateTexture2D(const std::string &path);
        static Ref<TextureCube> CreateTextureCube(const std::vector<std::string> &paths);
        static Ref<Shader> CreateShader(const std::string &filepath);
        static Ref<Shader> CreateShader(
                const std::string &name, const std::string &vertexSource, const std::string &fragmentSource);
        static Ref<Framebuffer> CreateFramebuffer(const FramebufferSpecification &specification);
        static Ref<UniformBuffer> CreateUniformBuffer(uint32_t size, uint32_t binding);
        /// nativeWindowHandle：当前 OpenGL 后端为 GLFWwindow*。
        static Scope<GraphicsContext> CreateGraphicsContext(void *nativeWindowHandle);

    private:
        static API s_API;
    };
}
