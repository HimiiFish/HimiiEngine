#pragma once
#include "Hepch.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Renderer/RenderCommand.h"
#include "Himii/Renderer/Shader.h"
#include "Himii/Renderer/VertexArray.h"
#include "glm/gtc/matrix_transform.hpp"

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

    struct Renderer2DData {
        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        Ref<VertexArray> QuadVertexArray;
        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<Shader> QuadShader;
        Ref<Texture2D> WhiteTexture;

        uint32_t QuadIndexCount = 0;
        QuadVertex *QuadVertexBufferBase = nullptr;
        QuadVertex *QuadVertexBufferPtr = nullptr;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1;

        glm::vec4 QuadVertexPositions[4];


        Renderer2D::Statistics Stats;

        struct CameraData {
            glm::mat4 ViewProjection;
        };
        CameraData CameraBuffer;
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

        s_Data.QuadVertexArray = VertexArray::Create();
        // 锟斤拷锟斤拷锟斤拷锟姐缓锟斤拷锟斤拷
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

        s_Data.WhiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        int32_t samplers[s_Data.MaxTextureSlots];
        for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
            samplers[i] = i;

        //  锟斤拷锟斤拷锟斤拷色锟斤拷锟斤拷锟斤拷
        s_Data.QuadShader = Shader::Create("assets/shaders/Texture.glsl");
        s_Data.QuadShader->Bind();
        s_Data.QuadShader->SetIntArray("u_Texture", samplers, s_Data.MaxTextureSlots);

        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        s_Data.QuadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};
    }
    void Renderer2D::Shutdown()
    {
        HIMII_PROFILE_FUNCTION();

        delete[] s_Data.QuadVertexBufferBase;
    }
    void Renderer2D::BeginScene(const OrthographicCamera &camera)
    {
        HIMII_PROFILE_FUNCTION();

        s_Data.QuadShader->Bind();
        s_Data.QuadShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
        s_Data.QuadShader->SetMat4("u_Transform", glm::mat4(1.0f));

        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

        s_Data.TextureSlotIndex = 1;
    }

    void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
    {
        HIMII_PROFILE_FUNCTION();
        s_Data.CameraBuffer.ViewProjection = camera.GetProjection()*glm::inverse(transform);
    }

    void Renderer2D::EndScene()
    {
        HIMII_PROFILE_FUNCTION();

        uint32_t dataSize = (uint8_t *)s_Data.QuadVertexBufferPtr - (uint8_t *)s_Data.QuadVertexBufferBase;
        s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

        Flush();
    }

    void Renderer2D::Flush()
    {
        for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
        {
            s_Data.TextureSlots[i]->Bind(i);
        }

        // s_Data.QuadShader->Bind();
        RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
        s_Data.Stats.DrawCalls++;
    }
    void Renderer2D::StartBatch()
    {
        EndScene();

        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

        s_Data.TextureSlotIndex = 1; // 0 reserved for white texture
    }

    // Some paths request a new batch when buffers/textures reach capacity
    void Renderer2D::NextBatch()
    {
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
        if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
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
        if (NeedsNewBatch(4, 6))
            NextBatch();
        const float textureIndex = 0.0f;
        const float tilingFactor = 1.0f;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                              glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f}) *
                              glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[0];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 0.0f};
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[1];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 0.0f};
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[2];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 1.0f};
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[3];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 1.0f};
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadIndexCount += 6;

        s_Data.Stats.QuadCount++;
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
        if (NeedsNewBatch(4, 6))
            NextBatch();
        constexpr glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

        float textureIndex = 0.0f;

        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            if (*s_Data.TextureSlots[i].get() == *texture.get())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            ++s_Data.TextureSlotIndex;
        }

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                              glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f}) *
                              glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[0];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 0.0f};
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[1];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 0.0f};
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[2];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 1.0f};
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[3];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 1.0f};
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadIndexCount += 6;

        s_Data.Stats.QuadCount++;
    }


    void Renderer2D::DrawSprite(const glm::mat4 &transform, SpriteRendererComponent &sprite, int entityID)
    {
        if (sprite.texture)
            DrawQuad(transform, sprite.texture, sprite.tilingFactor, sprite.color, entityID);
        else
            DrawQuad(transform, sprite.color, entityID);
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
