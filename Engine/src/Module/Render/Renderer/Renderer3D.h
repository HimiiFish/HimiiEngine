#pragma once
#include "Module/Render/Renderer/EditorCamera.h"
#include "Module/Render/RenderCore/Camera.h"
#include "Module/Render/RenderCore/Texture.h"

namespace Himii {

    class Renderer3D
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const Camera& camera, const glm::mat4& transform);
        static void EndScene();

        static void Flush();

        // Primitives
        static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID = -1);
        static void DrawCube(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

        static void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color, int entityID = -1);
        static void DrawSphere(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

        static void DrawCapsule(const glm::vec3& position, float radius, float height, const glm::vec4& color, int entityID = -1);
        static void DrawCapsule(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

        static void DrawPlane(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

        // Skybox
        static void DrawSkybox(const Ref<TextureCube> &cubemap, const Camera &camera, const glm::mat4 &cameraTransform);
        static void DrawSkybox(const Ref<TextureCube> &cubemap, const EditorCamera &camera);
        
        // Grid
        static void DrawGrid(const EditorCamera& camera, bool xyPlane = false);
        static void DrawGrid(const Camera& camera, const glm::mat4& transform, bool xyPlane = false);

        // Stats
        struct Statistics
        {
            uint32_t DrawCalls = 0;
            uint32_t CubeCount = 0;
            uint32_t QuadCount = 0;
            uint32_t SphereCount = 0;
            uint32_t CapsuleCount = 0;
            
            uint32_t TotalVertexCount = 0;
            uint32_t TotalIndexCount = 0;

            uint32_t GetTotalVertexCount() const { return TotalVertexCount; }
            uint32_t GetTotalIndexCount() const { return TotalIndexCount; }
        };

        static void ResetStats();
        static Statistics GetStatistics();

    private:
        static void StartBatch();
        static void NextBatch();
    };

}
