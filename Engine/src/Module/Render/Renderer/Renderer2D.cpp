#pragma once
#include "Hepch.h"
#include "Module/Render/Renderer/Renderer2D.h"
#include "Resource/SpriteSheetUtility.h"
#include "Module/Tilemap/TileMapCoordinateUtility.h"
#include "Module/Render/RHI/RenderCommand.h"
#include "Module/Render/RenderCore/Shader.h"
#include "Module/Render/RenderCore/UniformBuffer.h"
#include "Module/Render/RenderCore/VertexArray.h"
#include "Module/Render/Renderer/Font.h"
#include "Module/Render/Renderer/TextLayout.h"
#include "Module/Render/Renderer/FontRegressionTests.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Himii
{

    struct QuadVertex {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        float TexIndex;
        float TilingFactor;

        int EntityID;
    };

    struct CircleVertex
    {
        glm::vec3 WorldPosition;
        glm::vec3 LocalPosition;
        glm::vec4 Color;
        float Thickness;
        float Fade;

        int EntityID;
    };

    struct LineVertex {
        glm::vec3 Position;
        glm::vec4 Color;

        int EntityID;
    };

    struct TextVertex {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        float TexIndex;
        int EntityID;
    };

    struct Renderer2DData {
        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        Ref<VertexArray> QuadVertexArray;
        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<Shader> QuadShader;
        Ref<Texture2D> WhiteTexture;

        Ref<VertexArray> CircleVertexArray;
        Ref<VertexBuffer> CircleVertexBuffer;
        Ref<Shader> CircleShader;

        Ref<VertexArray> LineVertexArray;
        Ref<VertexBuffer> LineVertexBuffer;
        Ref<Shader> LineShader;

        Ref<VertexArray> TextVertexArray;
        Ref<VertexBuffer> TextVertexBuffer;
        Ref<Shader> TextShader;

        uint32_t QuadIndexCount = 0;
        QuadVertex *QuadVertexBufferBase = nullptr;
        QuadVertex *QuadVertexBufferPtr = nullptr;

        uint32_t CircleIndexCount = 0;
        CircleVertex *CircleVertexBufferBase = nullptr;
        CircleVertex *CircleVertexBufferPtr = nullptr;

        uint32_t LineVertexCount = 0;
        LineVertex *LineVertexBufferBase = nullptr;
        LineVertex *LineVertexBufferPtr = nullptr;

        uint32_t TextIndexCount = 0;
        TextVertex *TextVertexBufferBase = nullptr;
        TextVertex *TextVertexBufferPtr = nullptr;

        float LineWidth = 2.0f;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1;

        std::array<Ref<Texture2D>, MaxTextureSlots> FontAtlasSlots;
        uint32_t FontAtlasSlotIndex = 0;

        glm::vec4 QuadVertexPositions[4];

        Renderer2D::Statistics Stats;

        struct CameraData {
            glm::mat4 ViewProjection;
        };
        CameraData CameraBuffer;
        Ref<UniformBuffer> CameraUniformBuffer;
    };

    static Renderer2DData s_Data;

    // 计算当前批次是否需要换批（由调用者在类方法内触发 NextBatch）
    static inline bool NeedsNewBatch(uint32_t verticesNeeded, uint32_t indicesNeeded)
    {
        const uint32_t usedVertices = (uint32_t)(s_Data.QuadVertexBufferPtr - s_Data.QuadVertexBufferBase);
        return (usedVertices + verticesNeeded > Renderer2DData::MaxVertices) ||
               (s_Data.QuadIndexCount + indicesNeeded > Renderer2DData::MaxIndices);
    }

    void Renderer2D::Init()
    {
        HIMII_PROFILE_FUNCTION();

        //Quad
        s_Data.QuadVertexArray = VertexArray::Create();
        s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));

        s_Data.QuadVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_Position"},
                                            {ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Float2, "a_TexCoord"},
                                            {ShaderDataType::Float, "a_TexIndex"},
                                            {ShaderDataType::Float, "a_TilingFactor"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);
        
        s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

        uint32_t *quadIndices = new uint32_t[s_Data.MaxIndices];
        
        uint32_t offset = 0;
        for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
        {
            quadIndices[i + 0] = offset + 0;
            quadIndices[i + 1] = offset + 1;
            quadIndices[i + 2] = offset + 2;

            quadIndices[i + 3] = offset + 2;
            quadIndices[i + 4] = offset + 3;
            quadIndices[i + 5] = offset + 0;

            offset += 4;
        }

        Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
        s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
        delete[] quadIndices;

        //Circle
        s_Data.CircleVertexArray = VertexArray::Create();
        s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));

        s_Data.CircleVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_WorldPosition"},
                                              {ShaderDataType::Float3, "a_LocalPosition"},
                                            {ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Float, "a_Thickness"},
                                            {ShaderDataType::Float, "a_Fade"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
        s_Data.CircleVertexArray->SetIndexBuffer(quadIB);
        s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxVertices];

        // Line
        s_Data.LineVertexArray = VertexArray::Create();
        s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));

        s_Data.LineVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_Position"},
                                            {ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
        s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];

        s_Data.TextVertexArray = VertexArray::Create();
        s_Data.TextVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(TextVertex));

        s_Data.TextVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_Position"},
                                            {ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Float2, "a_TexCoord"},
                                            {ShaderDataType::Float, "a_TexIndex"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.TextVertexArray->AddVertexBuffer(s_Data.TextVertexBuffer);
        s_Data.TextVertexArray->SetIndexBuffer(quadIB);
        s_Data.TextVertexBufferBase = new TextVertex[s_Data.MaxVertices];

        s_Data.WhiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        int32_t samplers[s_Data.MaxTextureSlots];
        for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
            samplers[i] = i;

        s_Data.QuadShader = Shader::Create("assets/shaders/Renderer2D_Quad.glsl");
        s_Data.CircleShader = Shader::Create("assets/shaders/Renderer2D_Circle.glsl");
        s_Data.LineShader = Shader::Create("assets/shaders/Renderer2D_Line.glsl");
        s_Data.TextShader = Shader::Create("assets/shaders/Renderer2D_Text.glsl");
        HIMII_CORE_ASSERT(s_Data.TextShader && s_Data.TextShader->IsValid(),
                          "Text shader failed to create!");
        s_Data.TextShader->Bind();
        s_Data.TextShader->SetIntArray("u_FontAtlases", samplers, s_Data.MaxTextureSlots);

        FontRegression::RunTextLayoutSmokeTests();
        FontRegression::RunShaderValiditySmokeTest(s_Data.TextShader);

        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        s_Data.QuadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};

        s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 0);
    }
    void Renderer2D::Shutdown()
    {
        HIMII_PROFILE_FUNCTION();

        delete[] s_Data.QuadVertexBufferBase;
        delete[] s_Data.TextVertexBufferBase;
    }
    void Renderer2D::BeginScene(const OrthographicCamera &camera)
    {
        HIMII_PROFILE_FUNCTION();

        // 保持对正交相机的直接 uniform 支持，方便简单 2D 场景使用
        s_Data.QuadShader->Bind();
        s_Data.QuadShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
        s_Data.QuadShader->SetMat4("u_Transform", glm::mat4(1.0f));

        // 统一批次初始化逻辑
        StartBatch();
    }

    void Renderer2D::BeginScene(const EditorCamera& camera)
    {
        HIMII_PROFILE_FUNCTION();

        s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
        s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
        s_Data.CameraUniformBuffer->Bind();

        StartBatch();
    }

    void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
    {
        HIMII_PROFILE_FUNCTION();

        s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
        s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
        s_Data.CameraUniformBuffer->Bind();

        StartBatch();
    }

    void Renderer2D::EndScene()
    {
        HIMII_PROFILE_FUNCTION();

        Flush();
    }

    void Renderer2D::StartBatch()
    {

        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

        s_Data.CircleIndexCount = 0;
        s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

        s_Data.LineVertexCount = 0;
        s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

        s_Data.TextIndexCount = 0;
        s_Data.TextVertexBufferPtr = s_Data.TextVertexBufferBase;

        s_Data.TextureSlotIndex = 1; // 0 reserved for white texture
        s_Data.FontAtlasSlotIndex = 0;
    }

    void Renderer2D::Flush()
    {
        if (s_Data.QuadIndexCount)
        {
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.QuadVertexBufferPtr - (uint8_t *)s_Data.QuadVertexBufferBase);
            s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

            // Bind textures
            for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
                s_Data.TextureSlots[i]->Bind(i);

            s_Data.QuadShader->Bind();
            RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
            s_Data.Stats.DrawCalls++;
        }

        if (s_Data.CircleIndexCount)
        {
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.CircleVertexBufferPtr - (uint8_t *)s_Data.CircleVertexBufferBase);
            s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

            s_Data.CircleShader->Bind();
            RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
            s_Data.Stats.DrawCalls++;
        }

        if (s_Data.LineVertexCount)
        {
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.LineVertexBufferPtr - (uint8_t *)s_Data.LineVertexBufferBase);
            s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

            s_Data.LineShader->Bind();
            RenderCommand::SetLineWidth(s_Data.LineWidth);
            RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
            s_Data.Stats.DrawCalls++;
        }

        if (s_Data.TextIndexCount)
        {
            HIMII_CORE_ASSERT(s_Data.TextShader && s_Data.TextShader->IsValid(),
                              "Cannot flush text with invalid TextShader!");
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.TextVertexBufferPtr - (uint8_t *)s_Data.TextVertexBufferBase);
            s_Data.TextVertexBuffer->SetData(s_Data.TextVertexBufferBase, dataSize);

            for (uint32_t slotIndex = 0; slotIndex < s_Data.FontAtlasSlotIndex; ++slotIndex)
            {
                HIMII_CORE_ASSERT(s_Data.FontAtlasSlots[slotIndex], "Missing font atlas texture slot!");
                s_Data.FontAtlasSlots[slotIndex]->Bind(slotIndex);
            }

            s_Data.TextShader->Bind();
            RenderCommand::DrawIndexed(s_Data.TextVertexArray, s_Data.TextIndexCount);
            s_Data.Stats.DrawCalls++;
        }
    }

    // Some paths request a new batch when buffers/textures reach capacity
    void Renderer2D::NextBatch()
    {
        Flush();
        StartBatch();
    }

    //-----------------------------------锟斤拷锟斤拷锟侥憋拷锟斤拷----------------------------//
    void Renderer2D::DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, color);
    }
    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform =
                glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        DrawQuad(transform, color);
    }
    void Renderer2D::DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const Ref<Texture2D> &texture,
                              float tilingFactor, const glm::vec4 &tintColor)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, texture, tilingFactor, tintColor);
    }
    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const Ref<Texture2D> &texture,
                              float tilingFactor, const glm::vec4 &tintColor)
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform =
                glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        DrawQuad(transform, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::mat4 &transform, const glm::vec4 color, int entityID)
    {
        HIMII_PROFILE_FUNCTION();

        constexpr size_t quadVertexCount = 4;
        const float textureIndex = 0.0f; // White Texture
        constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        const float tilingFactor = 1.0f;

        if (NeedsNewBatch((uint32_t)quadVertexCount, 6))
            NextBatch();

        for (size_t i = 0; i < quadVertexCount; i++)
        {
            s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
            s_Data.QuadVertexBufferPtr->Color = color;
            s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            s_Data.QuadVertexBufferPtr->EntityID = entityID;
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;

        s_Data.Stats.QuadCount++;
    }

    void Renderer2D::DrawQuad(const glm::mat4 &transform, const Ref<Texture2D> &texture, float tilingFactor,
                              const glm::vec4 &tintColor, int entityID)
    {
        HIMII_PROFILE_FUNCTION();

        constexpr size_t quadVertexCount = 4;
        constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

        if (NeedsNewBatch((uint32_t)quadVertexCount, 6))
            NextBatch();

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            const auto &slotTex = s_Data.TextureSlots[i];
            if (slotTex && texture && *slotTex == *texture)
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();

            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
        }

        for (size_t i = 0; i < quadVertexCount; i++)
        {
            s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
            s_Data.QuadVertexBufferPtr->Color = tintColor;
            s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            s_Data.QuadVertexBufferPtr->EntityID = entityID;
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;

        s_Data.Stats.QuadCount++;
    }

    // Atlas-drawn quads with custom UVs
    void Renderer2D::DrawQuadUV(const glm::vec2 &position, const glm::vec2 &size, const Ref<Texture2D> &texture,
                                const std::array<glm::vec2, 4> &uvs, float tilingFactor, const glm::vec4 &tintColor)
    {
        DrawQuadUV(glm::vec3(position, 0.0f), size, texture, uvs, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuadUV(const glm::vec3 &position, const glm::vec2 &size, const Ref<Texture2D> &texture,
                                const std::array<glm::vec2, 4> &uvs, float tilingFactor, const glm::vec4 &tintColor)
    {
        HIMII_PROFILE_FUNCTION();
        if (NeedsNewBatch(4, 6))
            NextBatch();

        constexpr glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        float textureIndex = 0.0f;

        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            const auto &slotTex = s_Data.TextureSlots[i];
            if (slotTex && texture && *slotTex == *texture)
            {
                textureIndex = (float)i;
                break;
            }
        }
        if (textureIndex == 0.0f)
        {
            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            ++s_Data.TextureSlotIndex;
        }

        glm::mat4 transform =
                glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[0];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[0];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = -1;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[1];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[1];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = -1;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[2];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[2];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = -1;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[3];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[3];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = -1;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadIndexCount += 6;
        s_Data.Stats.QuadCount++;
    }

    // Transform-based UV variant
    void Renderer2D::DrawQuadUV(const glm::mat4 &transform, const Ref<Texture2D> &texture,
                                const std::array<glm::vec2, 4> &uvs, float tilingFactor, const glm::vec4 &tintColor,
                                int entityID)
    {
        HIMII_PROFILE_FUNCTION();
        if (NeedsNewBatch(4, 6))
            NextBatch();

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            const auto &slotTex = s_Data.TextureSlots[i];
            if (slotTex && texture && *slotTex == *texture)
            {
                textureIndex = (float)i;
                break;
            }
        }
        if (textureIndex == 0.0f)
        {
            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            ++s_Data.TextureSlotIndex;
        }

        constexpr size_t quadVertexCount = 4;
        const glm::vec4 color = tintColor;

        // Emit the 4 vertices
        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[0];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[0];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[1];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[1];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[2];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[2];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[3];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[3];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadIndexCount += 6;
        s_Data.Stats.QuadCount++;
    }

    //---------------------------------锟斤拷锟斤拷锟斤拷转锟侥憋拷锟斤拷------------------------------//
    void Renderer2D::DrawRotatedQuad(const glm::vec2 &position, const glm::vec2 &size, float rotation,
                                     const glm::vec4 &color)
    {
        DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, color);
    }
    void Renderer2D::DrawRotatedQuad(const glm::vec3 &position, const glm::vec2 &size, float rotation,
                                     const glm::vec4 &color)
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                              glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f}) *
                              glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        DrawQuad(transform, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2 &position, const glm::vec2 &size, float rotation,
                                     const Ref<Texture2D> &texture, float tilingFactor, const glm::vec4 &tintColor)
    {
        DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3 &position, const glm::vec2 &size, float rotation,
                                     const Ref<Texture2D> &texture, float tilingFactor, const glm::vec4 &tintColor)
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                              glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f}) *
                              glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        DrawQuad(transform, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4 &color, float thickness , float fade , int entityID)
    {
        HIMII_PROFILE_FUNCTION();

        // Circle 也通过批次渲染，确保不会溢出缓冲
        if (NeedsNewBatch(4, 6))
            NextBatch();

        for (size_t i = 0; i < 4; i++)
        {
            s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data.QuadVertexPositions[i];
            s_Data.CircleVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i]*2.0f;
            s_Data.CircleVertexBufferPtr->Color = color;
            s_Data.CircleVertexBufferPtr->Thickness = thickness;
            s_Data.CircleVertexBufferPtr->Fade = fade;
            s_Data.CircleVertexBufferPtr->EntityID = entityID;
            s_Data.CircleVertexBufferPtr++;
        }

        s_Data.CircleIndexCount += 6;

        // Circle 使用 Quad 批渲染，统计上仍然归入 QuadCount
        s_Data.Stats.QuadCount++;
    }

    void Renderer2D::DrawLine(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec4 &color, int entityID)
    {
        // 确保线段批次不会超过最大顶点数量
        if (s_Data.LineVertexCount + 2 > Renderer2DData::MaxVertices)
            NextBatch();

        s_Data.LineVertexBufferPtr->Position = p0;
        s_Data.LineVertexBufferPtr->Color = color;
        s_Data.LineVertexBufferPtr->EntityID = entityID;
        s_Data.LineVertexBufferPtr++;

        s_Data.LineVertexBufferPtr->Position = p1;
        s_Data.LineVertexBufferPtr->Color = color;
        s_Data.LineVertexBufferPtr->EntityID = entityID;
        s_Data.LineVertexBufferPtr++;

        s_Data.LineVertexCount += 2;
        s_Data.Stats.LineVertexCount += 2;
    }

    void Renderer2D::DrawRect(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color, int entityID)
    {
        glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
        glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
        glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
        glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

        DrawLine(p0, p1, color, entityID);
        DrawLine(p1, p2, color, entityID);
        DrawLine(p2, p3, color, entityID);
        DrawLine(p3, p0, color, entityID);
    }

    void Renderer2D::DrawRect(const glm::mat4 &transform, const glm::vec4 &color, int entityID)
    {
        glm::vec3 lineVertices[4];
        for (size_t i = 0; i < 4; i++)
            lineVertices[i] = transform * s_Data.QuadVertexPositions[i];

        DrawLine(lineVertices[0], lineVertices[1], color, entityID);
        DrawLine(lineVertices[1], lineVertices[2], color, entityID);
        DrawLine(lineVertices[2], lineVertices[3], color, entityID);
        DrawLine(lineVertices[3], lineVertices[0], color, entityID);
    }

    void Renderer2D::DrawTilemap(const glm::mat4 &transform, const Ref<TileMapData>& mapData, const Ref<TileSet>& tileSet, int entityID)
    {
        HIMII_PROFILE_FUNCTION();

        if (!mapData) return;

        const float cellSize = mapData->GetCellSize();
        if (cellSize <= 0.0f)
            return;

        if (tileSet)
        {
            if (auto assetManager = ResourceSystem::GetAssetManager())
            {
                for (auto &atlasSource : tileSet->GetAtlasSources())
                {
                    if (!atlasSource.CachedTexture && atlasSource.TextureHandle != 0
                        && assetManager->IsAssetHandleValid(atlasSource.TextureHandle))
                    {
                        atlasSource.CachedTexture =
                                std::static_pointer_cast<Texture2D>(assetManager->GetAsset(atlasSource.TextureHandle));
                    }
                }

                for (auto &[tileIdentifier, tileDefinition] : tileSet->GetTileDefs())
                {
                    if (tileDefinition.SourceType == TileSourceType::Individual
                        && !tileDefinition.CachedIndividualTexture
                        && tileDefinition.IndividualTextureHandle != 0
                        && assetManager->IsAssetHandleValid(tileDefinition.IndividualTextureHandle))
                    {
                        tileDefinition.CachedIndividualTexture = std::static_pointer_cast<Texture2D>(
                                assetManager->GetAsset(tileDefinition.IndividualTextureHandle));
                    }
                }
            }
        }

        auto acquireTextureSlot = [&](const Ref<Texture2D>& texture) -> float
        {
            if (!texture)
                return 0.0f;

            for (uint32_t slotIndex = 1; slotIndex < s_Data.TextureSlotIndex; slotIndex++)
            {
                if (s_Data.TextureSlots[slotIndex] && *s_Data.TextureSlots[slotIndex] == *texture)
                    return static_cast<float>(slotIndex);
            }

            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();

            const float textureSlotIndex = static_cast<float>(s_Data.TextureSlotIndex);
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
            return textureSlotIndex;
        };

        mapData->ForEachTile([&](int32_t tileX, int32_t tileY, uint16_t tileIdentifier)
        {
            if (NeedsNewBatch(4, 6))
                NextBatch();

            float textureIndex = 0.0f;
            glm::vec2 texCoords[4] = {
                {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
            glm::vec4 tint{1, 1, 1, 1};

            if (tileSet)
            {
                const TileDef* tileDefinition = tileSet->GetTileDef(tileIdentifier);
                if (tileDefinition)
                {
                    tint = tileDefinition->Tint;

                    if (tileDefinition->SourceType == TileSourceType::Atlas)
                    {
                        const auto& sources = tileSet->GetAtlasSources();
                        if (tileDefinition->AtlasSourceIndex < sources.size())
                        {
                            const auto& source = sources[tileDefinition->AtlasSourceIndex];
                            if (source.CachedTexture)
                            {
                                textureIndex = acquireTextureSlot(source.CachedTexture);

                                float tilePixelSize = (float)source.TileSize;
                                if (tilePixelSize <= 0.0f)
                                    tilePixelSize = 16.0f;

                                const std::array<glm::vec2, 4> atlasUVs =
                                        SpriteSheetUtility::AtlasGridCoordsToWorldQuadUVs(
                                                tileDefinition->AtlasCoords,
                                                static_cast<uint32_t>(tilePixelSize),
                                                source.CachedTexture->GetWidth(),
                                                source.CachedTexture->GetHeight());
                                for (size_t vertexIndex = 0; vertexIndex < 4; ++vertexIndex)
                                    texCoords[vertexIndex] = atlasUVs[vertexIndex];
                            }
                        }
                    }
                    else if (tileDefinition->SourceType == TileSourceType::Individual)
                    {
                        Ref<Texture2D> individualTexture = tileDefinition->CachedIndividualTexture;
                        if (individualTexture)
                            textureIndex = acquireTextureSlot(individualTexture);
                    }
                }
            }

            const glm::vec2 tileBottomLeft =
                    TileMapCoordinateUtility::TileLocalBottomLeft(tileX, tileY, cellSize);
            const float positionX = tileBottomLeft.x;
            const float positionY = tileBottomLeft.y;
            const glm::vec4 localPositions[4] = {
                {positionX, positionY, 0.0f, 1.0f},
                {positionX + cellSize, positionY, 0.0f, 1.0f},
                {positionX + cellSize, positionY + cellSize, 0.0f, 1.0f},
                {positionX, positionY + cellSize, 0.0f, 1.0f}};

            for (int vertexIndex = 0; vertexIndex < 4; vertexIndex++)
            {
                s_Data.QuadVertexBufferPtr->Position = transform * localPositions[vertexIndex];
                s_Data.QuadVertexBufferPtr->Color = tint;
                s_Data.QuadVertexBufferPtr->TexCoord = texCoords[vertexIndex];
                s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
                s_Data.QuadVertexBufferPtr->TilingFactor = 1.0f;
                s_Data.QuadVertexBufferPtr->EntityID = entityID;
                s_Data.QuadVertexBufferPtr++;
            }
            s_Data.QuadIndexCount += 6;
            s_Data.Stats.QuadCount++;
        });
    }

    static float GetFontAtlasTextureIndex(const Ref<Texture2D> &atlasTexture)
    {
        if (!atlasTexture)
            return 0.0f;

        for (uint32_t slotIndex = 0; slotIndex < s_Data.FontAtlasSlotIndex; ++slotIndex)
        {
            if (*s_Data.FontAtlasSlots[slotIndex] == *atlasTexture)
                return static_cast<float>(slotIndex);
        }

        if (s_Data.FontAtlasSlotIndex >= Renderer2DData::MaxTextureSlots)
            Renderer2D::FlushCurrentBatch();

        const float textureIndex = static_cast<float>(s_Data.FontAtlasSlotIndex);
        s_Data.FontAtlasSlots[s_Data.FontAtlasSlotIndex] = atlasTexture;
        ++s_Data.FontAtlasSlotIndex;
        return textureIndex;
    }

    static void EmitTextGlyphQuad(
            const glm::mat4 &transform, const glm::vec4 &color, int entityIdentifier,
            const glm::vec2 &minimum, const glm::vec2 &maximum,
            const glm::vec2 &textureMinimum, const glm::vec2 &textureMaximum,
            const Ref<Texture2D> &atlasTexture)
    {
        if (s_Data.TextIndexCount >= Renderer2DData::MaxIndices)
            Renderer2D::FlushCurrentBatch();

        const float textureIndex = GetFontAtlasTextureIndex(atlasTexture);
        const glm::vec4 localPositions[4] = {
                {minimum.x, minimum.y, 0.0f, 1.0f},
                {maximum.x, minimum.y, 0.0f, 1.0f},
                {maximum.x, maximum.y, 0.0f, 1.0f},
                {minimum.x, maximum.y, 0.0f, 1.0f}};
        const glm::vec2 textureCoordinates[4] = {
                textureMinimum,
                {textureMaximum.x, textureMinimum.y},
                textureMaximum,
                {textureMinimum.x, textureMaximum.y}};

        for (size_t vertexIndex = 0; vertexIndex < 4; ++vertexIndex)
        {
            s_Data.TextVertexBufferPtr->Position = transform * localPositions[vertexIndex];
            s_Data.TextVertexBufferPtr->Color = color;
            s_Data.TextVertexBufferPtr->TexCoord = textureCoordinates[vertexIndex];
            s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
            s_Data.TextVertexBufferPtr->EntityID = entityIdentifier;
            ++s_Data.TextVertexBufferPtr;
        }
        s_Data.TextIndexCount += 6;
        ++s_Data.Stats.QuadCount;
    }

    void Renderer2D::DrawString(const std::string &string, Ref<Font> font, const glm::mat4 &transform,
                                const glm::vec4 &color, int entityID)
    {
        if (!font || string.empty() || !s_Data.TextShader || !s_Data.TextShader->IsValid())
            return;

        font->EnsureGlyphsForText(string);
        const FontMetrics &metrics = font->GetMetrics();
        const float fontScale =
                1.0f / std::max(metrics.AscenderY - metrics.DescenderY, 0.0001f);
        float cursorX = 0.0f;
        float cursorY = 0.0f;

        const std::vector<char32_t> codePoints = TextShaper::DecodeUtf8(string);
        for (size_t index = 0; index < codePoints.size(); ++index)
        {
            const char32_t character = codePoints[index];
            if (character == U'\n')
            {
                cursorX = 0.0f;
                cursorY -= fontScale * metrics.LineHeight;
                continue;
            }

            const char32_t nextCharacter =
                    index + 1 < codePoints.size() ? codePoints[index + 1] : U'\0';
            FontGlyphQuad glyph;
            if (!font->TryGetGlyph(character, glyph) || !glyph.Valid)
                continue;

            if (!glyph.IsWhitespace && glyph.AtlasPageTexture)
            {
                const glm::vec2 minimum = glyph.PlaneMinimum * fontScale + glm::vec2(cursorX, cursorY);
                const glm::vec2 maximum = glyph.PlaneMaximum * fontScale + glm::vec2(cursorX, cursorY);
                EmitTextGlyphQuad(
                        transform, color, entityID, minimum, maximum, glyph.AtlasUVMinimum,
                        glyph.AtlasUVMaximum, glyph.AtlasPageTexture);
            }

            cursorX += font->GetAdvance(character, nextCharacter) * fontScale;
        }
    }

    void Renderer2D::DrawStringInRectangle(
            const std::string &string, const Ref<Font> &font,
            const glm::mat4 &transform, const TextLayoutSettings &layoutSettings,
            const glm::vec4 &color, int entityIdentifier)
    {
        if (!font || string.empty() || layoutSettings.FontSize <= 0.0f
            || layoutSettings.RectangleSize.x <= 0.0f
            || layoutSettings.RectangleSize.y <= 0.0f
            || !s_Data.TextShader || !s_Data.TextShader->IsValid())
            return;

        font->EnsureGlyphsForText(string);
        const FontMetrics &metrics = font->GetMetrics();
        // FontSize = em 字号（设计像素）
        const float fontScale = layoutSettings.FontSize;
        const float lineHeight = std::max(
                metrics.LineHeight * fontScale + layoutSettings.LineSpacing, 0.01f);
        const std::vector<char32_t> codePoints = TextShaper::DecodeUtf8(string);

        const auto getAdvance = [&](char32_t codePoint, char32_t nextCodePoint)
        {
            return font->GetAdvance(codePoint, nextCodePoint) * fontScale
                   + (nextCodePoint != U'\0' ? layoutSettings.Kerning : 0.0f);
        };
        const auto measureCodePoints =
                [&](const std::vector<char32_t> &measuredCodePoints)
        {
            float width = 0.0f;
            for (size_t codePointIndex = 0; codePointIndex < measuredCodePoints.size();
                 ++codePointIndex)
            {
                const char32_t nextCodePoint =
                        codePointIndex + 1 < measuredCodePoints.size()
                                ? measuredCodePoints[codePointIndex + 1]
                                : U'\0';
                width += getAdvance(measuredCodePoints[codePointIndex], nextCodePoint);
            }
            return width;
        };

        struct TextLine
        {
            std::vector<char32_t> CodePoints;
            float Width = 0.0f;
        };

        std::vector<TextLine> lines;
        TextLine currentLine;
        const auto finishCurrentLine = [&]()
        {
            currentLine.Width = measureCodePoints(currentLine.CodePoints);
            lines.push_back(currentLine);
            currentLine = {};
        };

        for (size_t codePointIndex = 0; codePointIndex < codePoints.size();)
        {
            if (codePoints[codePointIndex] == U'\n')
            {
                finishCurrentLine();
                ++codePointIndex;
                continue;
            }

            std::vector<char32_t> token;
            const char32_t firstCodePoint = codePoints[codePointIndex];
            if (firstCodePoint == U' ' || firstCodePoint == U'\t'
                || TextShaper::IsChineseJapaneseKoreanCodePoint(firstCodePoint))
            {
                token.push_back(firstCodePoint);
                ++codePointIndex;
            }
            else
            {
                while (codePointIndex < codePoints.size())
                {
                    const char32_t codePoint = codePoints[codePointIndex];
                    if (codePoint == U'\n' || codePoint == U' ' || codePoint == U'\t'
                        || TextShaper::IsChineseJapaneseKoreanCodePoint(codePoint))
                        break;
                    token.push_back(codePoint);
                    ++codePointIndex;
                }
            }

            if (token.empty())
                continue;
            if (!currentLine.CodePoints.empty()
                && measureCodePoints(currentLine.CodePoints) + measureCodePoints(token)
                           > layoutSettings.RectangleSize.x)
            {
                finishCurrentLine();
            }
            if (currentLine.CodePoints.empty()
                && (token.front() == U' ' || token.front() == U'\t'))
                continue;

            for (char32_t codePoint : token)
            {
                std::vector<char32_t> candidate = currentLine.CodePoints;
                candidate.push_back(codePoint);
                if (!currentLine.CodePoints.empty()
                    && measureCodePoints(candidate) > layoutSettings.RectangleSize.x)
                {
                    finishCurrentLine();
                }
                currentLine.CodePoints.push_back(codePoint);
            }
        }
        if (!currentLine.CodePoints.empty() || lines.empty())
            finishCurrentLine();

        const float textBlockHeight =
                layoutSettings.FontSize + lineHeight * static_cast<float>(lines.size() - 1);
        const float ascenderHeight = metrics.AscenderY * fontScale;
        float firstBaseline = 0.0f;
        if (layoutSettings.VerticalAlignment == TextVerticalAlignment::Top)
            firstBaseline = layoutSettings.RectangleSize.y * 0.5f - ascenderHeight;
        else if (layoutSettings.VerticalAlignment == TextVerticalAlignment::Middle)
            firstBaseline = textBlockHeight * 0.5f - ascenderHeight;
        else
            firstBaseline = -layoutSettings.RectangleSize.y * 0.5f + textBlockHeight
                            - ascenderHeight;

        const glm::vec2 rectangleMinimum = -layoutSettings.RectangleSize * 0.5f;
        const glm::vec2 rectangleMaximum = layoutSettings.RectangleSize * 0.5f;

        for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
        {
            const TextLine &line = lines[lineIndex];
            float cursorX = rectangleMinimum.x;
            if (layoutSettings.HorizontalAlignment == TextHorizontalAlignment::Center)
                cursorX = -line.Width * 0.5f;
            else if (layoutSettings.HorizontalAlignment == TextHorizontalAlignment::Right)
                cursorX = rectangleMaximum.x - line.Width;
            const float baselineY = firstBaseline - static_cast<float>(lineIndex) * lineHeight;

            for (size_t characterIndex = 0; characterIndex < line.CodePoints.size();
                 ++characterIndex)
            {
                const char32_t character = line.CodePoints[characterIndex];
                const char32_t nextCharacter =
                        characterIndex + 1 < line.CodePoints.size()
                                ? line.CodePoints[characterIndex + 1]
                                : U'\0';

                FontGlyphQuad glyph;
                Ref<Font> resolvedFont = Font::ResolveWithFallback(font, character);
                if (!resolvedFont || !resolvedFont->TryGetGlyph(character, glyph) || !glyph.Valid)
                {
                    cursorX += getAdvance(character, nextCharacter);
                    continue;
                }

                if (!glyph.IsWhitespace && glyph.AtlasPageTexture)
                {
                    const glm::vec2 originalMinimum = {
                            cursorX + glyph.PlaneMinimum.x * fontScale,
                            baselineY + glyph.PlaneMinimum.y * fontScale};
                    const glm::vec2 originalMaximum = {
                            cursorX + glyph.PlaneMaximum.x * fontScale,
                            baselineY + glyph.PlaneMaximum.y * fontScale};
                    const glm::vec2 clippedMinimum = glm::max(originalMinimum, rectangleMinimum);
                    const glm::vec2 clippedMaximum = glm::min(originalMaximum, rectangleMaximum);

                    if (clippedMinimum.x < clippedMaximum.x && clippedMinimum.y < clippedMaximum.y)
                    {
                        const glm::vec2 originalSize = originalMaximum - originalMinimum;
                        const glm::vec2 minimumRatio =
                                (clippedMinimum - originalMinimum) / originalSize;
                        const glm::vec2 maximumRatio =
                                (clippedMaximum - originalMinimum) / originalSize;
                        const glm::vec2 clippedTextureMinimum =
                                glm::mix(glyph.AtlasUVMinimum, glyph.AtlasUVMaximum, minimumRatio);
                        const glm::vec2 clippedTextureMaximum =
                                glm::mix(glyph.AtlasUVMinimum, glyph.AtlasUVMaximum, maximumRatio);
                        EmitTextGlyphQuad(
                                transform, color, entityIdentifier, clippedMinimum,
                                clippedMaximum, clippedTextureMinimum, clippedTextureMaximum,
                                glyph.AtlasPageTexture);
                    }
                }

                cursorX += getAdvance(character, nextCharacter);
            }
        }
    }

    void Renderer2D::DrawSprite(const glm::mat4 &transform,
                                const SpriteRendererComponent &sprite,
                                const SpriteResolved &resolved,
                                int entityID)
    {
        if (!resolved.IsValid || !resolved.Texture)
        {
            DrawQuad(transform, sprite.Color, entityID);
            return;
        }

        SpriteSheetUtility::SpriteFacingCorrection facingCorrection =
                SpriteSheetUtility::CorrectTransformNegativeScaleForSprite(transform);
        if (sprite.FlipHorizontal)
            facingCorrection.FlipUVHorizontal = !facingCorrection.FlipUVHorizontal;

        const glm::mat4 renderTransform = SpriteSheetUtility::BuildSpriteRenderTransform(
                facingCorrection.RenderTransform, resolved.PixelSize, resolved.PixelsPerUnit,
                resolved.Pivot);

        const bool usesSubRegion = resolved.PixelSize.x > 0 && resolved.PixelSize.y > 0;
        const float tilingFactor = usesSubRegion ? 1.0f : sprite.TilingFactor;

        if (usesSubRegion)
        {
            const std::array<glm::vec2, 4> correctedUVs =
                    SpriteSheetUtility::ApplySpriteUvFacing(resolved.UVs, facingCorrection);
            DrawQuadUV(renderTransform, resolved.Texture, correctedUVs, tilingFactor, sprite.Color,
                       entityID);
        }
        else
        {
            const glm::ivec4 fullTextureRect{
                0, 0,
                static_cast<int>(resolved.Texture->GetWidth()),
                static_cast<int>(resolved.Texture->GetHeight())};
            std::array<glm::vec2, 4> fullTextureUVs = SpriteSheetUtility::PixelRectToWorldQuadUVs(
                    fullTextureRect, resolved.Texture->GetWidth(), resolved.Texture->GetHeight());
            fullTextureUVs =
                    SpriteSheetUtility::ApplySpriteUvFacing(fullTextureUVs, facingCorrection);
            DrawQuadUV(renderTransform, resolved.Texture, fullTextureUVs, tilingFactor, sprite.Color,
                       entityID);
        }
    }

    float Renderer2D::GetLineWidth()
    {
        return s_Data.LineWidth;
    }

    void Renderer2D::SetLineWidth(float width)
    {
        s_Data.LineWidth = width;
    }

    void Renderer2D::ResetStats()
    {
        memset(&s_Data.Stats, 0, sizeof(Statistics));
    }
    Renderer2D::Statistics Renderer2D::GetStatistics()
    {
        return s_Data.Stats;
    }
} // namespace Himii
