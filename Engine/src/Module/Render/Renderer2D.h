#pragma once
#include "Module/Render/OrthographicCamera.h"
#include "Module/Render/Texture.h"

#include "Resource/Sprite.h"
#include "World/Scene/Components.h"
#include "Module/Tilemap/TileSet.h"
#include "Module/Tilemap/TileMapData.h"
#include "Module/Render/EditorCamera.h"
#include "Module/Render/Font.h"

namespace Himii
{
    class Renderer2D {
    public:

        static void Init();
        static void Shutdown();

        static void BeginScene(const OrthographicCamera &camera);
        static void BeginScene(const EditorCamera &camera);
        static void BeginScene(const Camera &camera,const glm::mat4& transform);
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

        static void DrawCircle(const glm::mat4 &transform, const glm::vec4 &color,float thickness=1.0f,float fade=0.0025f, int entityID = -1);

        static void DrawLine(const glm::vec3 &p0,const glm::vec3&p1, const glm::vec4 &color, int entityID = -1);

        static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
		static void DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

        static void DrawSprite(const glm::mat4 &transform,
                               const SpriteRendererComponent& sprite,
                               const SpriteResolved& resolved,
                               int entityID = -1);
        
        static void DrawTilemap(const glm::mat4 &transform, const Ref<TileMapData>& mapData, const Ref<TileSet>& tileSet, int entityID = -1);

        static void DrawString(const std::string &string, Ref<Font> font, const glm::mat4 &transform,const glm::vec4 &color, int entityID = -1);
        static void DrawStringInRectangle(
                const std::string& string, const Ref<Font>& font,
                const glm::mat4& transform, const TextLayoutSettings& layoutSettings,
                const glm::vec4& color, int entityIdentifier = -1);

        static float GetLineWidth();
        static void SetLineWidth(float width);

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
            // Circle 也是用 Quad 批渲染，这里不单独计数
            uint32_t LineVertexCount = 0;

            uint32_t GetTotalVertexCount() const
            {
                return QuadCount * 4;
            }
            uint32_t GetTotalIndexCount() const
            {
                return QuadCount * 6;
            }

            uint32_t GetLineCount() const
            {
                return LineVertexCount / 2;
            }
        };

        static void ResetStats();
        static Statistics GetStatistics();

        // 供同编译单元批处理辅助函数在槽位耗尽时换批。
        static void FlushCurrentBatch()
        {
            NextBatch();
        }

        private:
            static void StartBatch();
            static void NextBatch();
    };
} // namespace Himii
