#pragma once
#include "Module/Render/Renderer/EditorCamera.h"
#include "Module/Render/RenderCore/Camera.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Resource/Asset.h"

#include <vector>

namespace Himii {

    class MeshAsset;

    inline constexpr uint32_t ScenePointLightCapacity = 8u;

    struct PointLightParameters
    {
        glm::vec3 Position{0.0f};
        float Range = 10.0f;
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        float Intensity = 1.0f;
    };

    struct SceneLightingParameters
    {
        bool HasDirectionalLight = false;
        /// 光线传播方向（从光源指向被照表面），由 Transform forward（局部 -Z）推导。
        glm::vec3 DirectionalLightDirection{0.0f, -1.0f, 0.0f};
        glm::vec3 DirectionalLightColor{1.0f, 1.0f, 1.0f};
        float DirectionalLightIntensity = 1.0f;
        glm::vec3 AmbientColor{0.0f, 0.0f, 0.0f};
        float AmbientIntensity = 0.0f;

        uint32_t PointLightCount = 0;
        PointLightParameters PointLights[ScenePointLightCapacity]{};

        bool HasShadowMap = false;
        glm::mat4 LightViewProjection{1.0f};
        /// 引擎默认常量 Bias；不进 Inspector。
        float ShadowBias = 0.0015f;
        /// 单个阴影贴图像素在世界空间中的边长，供 shader 做 normal-offset 偏移。
        float ShadowTexelWorldSize = 0.0f;
    };

    class Renderer3D
    {
    public:
        static void Init();
        static void Shutdown();

        static void SetSceneLighting(const SceneLightingParameters &parameters);
        static SceneLightingParameters GetSceneLighting();

        /// 按分辨率创建或重建单张深度 Shadow Map。
        static void EnsureShadowMap(uint32_t resolutionPixels);
        /// 绑定 Shadow Map、写入光空间 VP，随后 Draw* 仅输出深度。
        static void BeginShadowPass(const glm::mat4 &lightViewProjection);
        static void EndShadowPass();

        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const Camera& camera, const glm::mat4& transform);
        static void EndScene();

        static void Flush();

        // Primitives（Color 为 Albedo；无材质时由调用方传入默认 Specular/Shininess）
        static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID = -1);
        static void DrawCube(const glm::mat4& transform, const glm::vec4& color, int entityID = -1,
                             float specular = 0.5f, float shininess = 32.0f,
                             const Ref<Texture2D> &albedoTexture = nullptr);

        static void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color, int entityID = -1);
        static void DrawSphere(const glm::mat4& transform, const glm::vec4& color, int entityID = -1,
                               float specular = 0.5f, float shininess = 32.0f,
                               const Ref<Texture2D> &albedoTexture = nullptr);

        static void DrawCapsule(const glm::vec3& position, float radius, float height, const glm::vec4& color, int entityID = -1);
        static void DrawCapsule(const glm::mat4& transform, const glm::vec4& color, int entityID = -1,
                                float specular = 0.5f, float shininess = 32.0f,
                                const Ref<Texture2D> &albedoTexture = nullptr);

        static void DrawPlane(const glm::mat4& transform, const glm::vec4& color, int entityID = -1,
                              float specular = 0.5f, float shininess = 32.0f,
                              const Ref<Texture2D> &albedoTexture = nullptr);

        /// 按 submesh 提交网格；默认 Lit，材质标记 Unlit 时走 Unlit 回退。
        static void DrawMeshAsset(const Ref<MeshAsset> &meshAsset,
                                  const std::vector<AssetHandle> &materialAssetHandles,
                                  const glm::mat4 &transform,
                                  int entityID = -1);

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
        static void UploadCameraAndLighting();
        static void BindShadowMapIfAvailable();
        static float ResolveTextureIndex(const Ref<Texture2D> &albedoTexture);
    };

}
