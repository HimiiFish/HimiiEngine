#pragma once
#include "Himii/Renderer/OrthographicCamera.h"
#include "Himii/Renderer/Texture.h"

#include "Himii/Scene/Components.h"

namespace Himii
{
    class Renderer2D {
    public:

        static void Init();
        static void Shutdown();

        static void BeginScene(const OrthographicCamera &camera);
        static void EndScene();

        static void Flush();
        //--yuan
        static void DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const glm::vec4 &color);
        static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color);
        static void DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const Ref<Texture2D>& texture,float tilingFactor=1.0f,const glm::vec4& tintColor =glm::vec4(1.0f));
        static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const Ref<Texture2D>& texture,float tilingFactor=1.0f,const glm::vec4& tintColor =glm::vec4(1.0f));

        static void DrawQuad(const glm::mat4 &transform, const glm::vec4 color,int entityID=-1);
        static void DrawQuad(const glm::mat4 &transform, const Ref<Texture2D>& texture,float tilingFactor=1.0f,const glm::vec4& tintColor=glm::vec4(1.0f),int entityID=-1);

        static void DrawRotatedQuad(const glm::vec2 &position, const glm::vec2 &size,float rotation, const glm::vec4& color);
        static void DrawRotatedQuad(const glm::vec3 &position, const glm::vec2 &size,float rotation, const glm::vec4& color);
        static void DrawRotatedQuad(const glm::vec2 &position, const glm::vec2 &size,float rotation, const Ref<Texture2D>& texture,float tilingFactor=1.0f,const glm::vec4& tintColor=glm::vec4(1.0f));
        static void DrawRotatedQuad(const glm::vec3 &position, const glm::vec2 &size,float rotation, const Ref<Texture2D>& texture,float tilingFactor=1.0f,const glm::vec4& tintColor=glm::vec4(1.0f));

        static void DrawSprite(const glm::mat4 &transform, SpriteRenderer& sprite,int entityID=-1);


    // Atlas helpers: draw a quad textured with custom UVs (e.g., from a sprite atlas)
    static void DrawQuadUV(const glm::vec2 &position, const glm::vec2 &size,
                   const Ref<Texture2D>& texture,
                   const std::array<glm::vec2,4>& uvs,
                   float tilingFactor=1.0f,
                   const glm::vec4& tintColor = glm::vec4(1.0f));
    static void DrawQuadUV(const glm::vec3 &position, const glm::vec2 &size,
                   const Ref<Texture2D>& texture,
                   const std::array<glm::vec2,4>& uvs,
                   float tilingFactor=1.0f,
                   const glm::vec4& tintColor = glm::vec4(1.0f));

    // Transform-based UV variant: supports full Position/Rotation/Scale via transform matrix
    static void DrawQuadUV(const glm::mat4 &transform,
                   const Ref<Texture2D>& texture,
                   const std::array<glm::vec2,4>& uvs,
                   float tilingFactor=1.0f,
                   const glm::vec4& tintColor = glm::vec4(1.0f),
                   int entityID = -1);

        struct Statistics {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;

            uint32_t GetTotalVertexCount() const
            {
                return QuadCount * 4;
            }
            uint32_t GetTotalIndexCount() const
            {
                return QuadCount * 6;
            }
        };

        static void ResetStats();
        static Statistics GetStatistics();

        private:
            static void StartBatch();
            static void NextBatch();
    };
} // namespace Himii
