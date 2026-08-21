#include "Hepch.h"
#include "Module/Render/Renderer/SceneColorResolvePass.h"
#include "Module/Render/RenderCore/Framebuffer.h"
#include "Module/Render/RenderCore/Shader.h"
#include "Module/Render/RenderCore/UniformBuffer.h"
#include "Module/Render/RenderCore/VertexArray.h"
#include "Module/Render/RHI/RenderCommand.h"
#include "Module/Render/RHI/RHI.h"

#include <algorithm>
#include <glm/glm.hpp>

namespace Himii
{
    namespace
    {
        struct SceneColorResolveUniformsData
        {
            glm::vec4 ExposureParameters{1.0f, 0.0f, 0.0f, 0.0f};
        };

        struct SceneColorResolvePassData
        {
            Ref<Shader> ResolveShader;
            Ref<VertexArray> FullscreenVertexArray;
            Ref<UniformBuffer> ResolveUniformBuffer;
            bool Initialized = false;
        };

        SceneColorResolvePassData &GetSceneColorResolvePassData()
        {
            static SceneColorResolvePassData data;
            return data;
        }
    }

    float SceneColorResolvePass::ClampExposure(float exposure)
    {
        return std::max(exposure, 0.001f);
    }

    void SceneColorResolvePass::Init()
    {
        SceneColorResolvePassData &data = GetSceneColorResolvePassData();
        if (data.Initialized)
            return;

        data.ResolveShader = Shader::Create("assets/shaders/SceneColorResolve.glsl");
        data.FullscreenVertexArray = VertexArray::Create();
        data.ResolveUniformBuffer =
                UniformBuffer::Create(sizeof(SceneColorResolveUniformsData), 5);
        data.Initialized = true;
    }

    void SceneColorResolvePass::Shutdown()
    {
        SceneColorResolvePassData &data = GetSceneColorResolvePassData();
        data.ResolveShader.reset();
        data.FullscreenVertexArray.reset();
        data.ResolveUniformBuffer.reset();
        data.Initialized = false;
    }

    void SceneColorResolvePass::Resolve(const Ref<Framebuffer> &sourceFramebuffer, float exposure)
    {
        if (!sourceFramebuffer)
            return;

        SceneColorResolvePassData &data = GetSceneColorResolvePassData();
        if (!data.Initialized)
            Init();
        if (!data.ResolveShader || !data.ResolveShader->IsValid() || !data.FullscreenVertexArray
            || !data.ResolveUniformBuffer)
            return;

        RenderCommand::SetDepthTest(false);
        RenderCommand::SetDepthMask(false);
        RenderCommand::SetCullMode(RHI::CullMode::None);

        SceneColorResolveUniformsData uniforms{};
        uniforms.ExposureParameters.x = ClampExposure(exposure);
        data.ResolveUniformBuffer->SetData(&uniforms, sizeof(SceneColorResolveUniformsData));
        data.ResolveUniformBuffer->Bind();

        data.ResolveShader->Bind();
        sourceFramebuffer->BindColorAttachment(0, 0);

        data.FullscreenVertexArray->Bind();
        RenderCommand::DrawArrays(data.FullscreenVertexArray, 3);

        RenderCommand::SetCullMode(RHI::CullMode::Back);
        RenderCommand::SetDepthMask(true);
        RenderCommand::SetDepthTest(true);
    }
}
