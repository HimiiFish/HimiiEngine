#pragma once
#include "Module/Render/Renderer/EditorCamera.h"
#include "Module/Render/RenderCore/Camera.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Resource/Asset.h"

#include <vector>

namespace Himii {

    class MeshAsset;
    class VertexArray;

    inline constexpr uint32_t ScenePointLightCapacity = 8u;
    inline constexpr uint32_t DirectionalCascadedShadowCascadeCount = 4u;

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
        glm::mat4 LightViewProjection[DirectionalCascadedShadowCascadeCount]{glm::mat4(1.0f), glm::mat4(1.0f),
                                                                            glm::mat4(1.0f), glm::mat4(1.0f)};
        /// 引擎默认常量 Bias；不进 Inspector。
        float ShadowBias = 0.0015f;
        /// xyz = 级联内部分割距离（沿观察前向的正距离），w = 最大阴影距离。
        glm::vec4 CascadeSplitDistances{0.0f};
        /// 四个级联各自的世界空间纹素边长，供 shader 做 normal-offset。
        glm::vec4 ShadowTexelWorldSize{0.0f};
        glm::vec3 ShadowViewerForwardDirection{0.0f, 0.0f, -1.0f};
        float ShadowCascadeOverlapRatio = 0.10f;
        /// 级联覆盖起点（沿观察前向的正距离，通常为相机近平面）。
        float ShadowCascadeNearDistance = 0.05f;
        /// 1 / atlas 边长，供 shader 把 clip UV 映射到带 padding 的 atlas 分块。
        float ShadowAtlasTexelUvSize = 0.0f;

        bool HasImageBasedLighting = false;
        float EnvironmentIntensity = 1.0f;
        float PrefilterMipCount = 1.0f;
    };

    class Renderer3D
    {
    public:
        static void Init();
        static void Shutdown();

        static void SetSceneLighting(const SceneLightingParameters &parameters);
        static SceneLightingParameters GetSceneLighting();

        /// 绑定 Split-Sum IBL 贴图；intensity 写入 SceneLighting UBO。
        static void SetImageBasedLighting(const Ref<TextureCube> &irradianceCubemap,
                                          const Ref<TextureCube> &prefilteredCubemap,
                                          const Ref<Texture2D> &brdfLookupTexture, float intensity,
                                          float prefilterMipCount);
        static void ClearImageBasedLighting();

        /// 按分辨率创建或重建单张深度 Shadow Atlas。
        static void EnsureShadowMap(uint32_t resolutionPixels);
        /// 绑定 Shadow Atlas、清深度、双面投射；随后按级联调用 SetShadowCascadeViewProjection。
        static void BeginShadowPass();
        /// 设置当前级联的光空间 VP 与 atlas 分块 viewport，并开启新的深度批次。
        static void SetShadowCascadeViewProjection(const glm::mat4 &lightViewProjection, uint32_t viewportX,
                                                   uint32_t viewportY, uint32_t viewportWidth,
                                                   uint32_t viewportHeight);
        static void EndShadowPass();

        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const Camera& camera, const glm::mat4& transform);
        static void EndScene();

        static void Flush();

        /// 场景内置原语（Cube / Sphere / Plane / Capsule）走 MeshLit / Unlit，与静态网格同一套材质。
        enum class BuiltinLitPrimitive
        {
            Cube = 0,
            Plane = 1,
            Sphere = 2,
            Capsule = 3
        };

        static void DrawBuiltinLitMesh(BuiltinLitPrimitive primitive, const glm::mat4 &transform,
                                       AssetHandle materialHandle, int entityID = -1);

        /// 调试用实例化原语（Phong）。场景网格请用 DrawBuiltinLitMesh。
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
        static void SubmitMaterialGeometry(const Ref<VertexArray> &vertexArray, uint32_t indexCount,
                                           const glm::mat4 &transform, AssetHandle materialHandle, int entityID);
    };

}
