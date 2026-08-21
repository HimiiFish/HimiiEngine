#include "Hepch.h"

#if defined(_WIN32)
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#endif

#include "Renderer3D.h"
#include "World/Scene/SceneCamera.h"

#include "Module/Render/RHI/RenderCommand.h"
#include "Module/Render/RenderCore/Framebuffer.h"
#include "Module/Render/RenderCore/Shader.h"
#include "Module/Render/RenderCore/UniformBuffer.h"
#include "Module/Render/RenderCore/VertexArray.h"
#include "Module/Render/Mesh/MeshAsset.h"
#include "Module/Render/Mesh/MaterialAsset.h"
#include "Module/Render/Mesh/MaterialSurfaceUtility.h"
#include "Module/Render/Environment/EnvironmentLightingSystem.h"
#include "Resource/ResourceSystem.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>

namespace Himii
{
    // [修改] 静态顶点数据 (Per Vertex)
    struct UnitVertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
    };

    // Packed to 16-byte alignment to match OpenGL layout and avoid padding issues.
    struct InstanceData {
        glm::vec4 Color;
        glm::vec4 CustomData; // x = TexIndex, y = EntityID, z = Specular, w = Shininess
        glm::vec4 TransformRow0;
        glm::vec4 TransformRow1;
        glm::vec4 TransformRow2;
        glm::vec4 TransformRow3;
    };

    struct Renderer3DData {
        static constexpr uint32_t MaxInstances = 10000; // Max instances per batch

        // Shared Instance Buffer (Dynamic GPU Resource)
        Ref<VertexBuffer> InstanceVertexBuffer;

        // Cube
        Ref<VertexArray> CubeVAO;
        Ref<VertexBuffer> CubeVBO; 
        Scope<InstanceData[]> CubeInstanceBase;
        InstanceData* CubeInstancePtr = nullptr;
        uint32_t CubeInstanceCount = 0;

        // Plane
        Ref<VertexArray> PlaneVAO;
        Ref<VertexBuffer> PlaneVBO;
        Scope<InstanceData[]> PlaneInstanceBase;
        InstanceData* PlaneInstancePtr = nullptr;
        uint32_t PlaneInstanceCount = 0;

        // Sphere
        Ref<VertexArray> SphereVAO;
        Ref<VertexBuffer> SphereVBO;
        Scope<InstanceData[]> SphereInstanceBase;
        InstanceData* SphereInstancePtr = nullptr;
        uint32_t SphereInstanceCount = 0;
        uint32_t SphereIndexCount = 0;
        uint32_t SphereVertexCount = 0; // Added for stats

        // Capsule
        Ref<VertexArray> CapsuleVAO;
        Ref<VertexBuffer> CapsuleVBO;
        Scope<InstanceData[]> CapsuleInstanceBase;
        InstanceData* CapsuleInstancePtr = nullptr;
        uint32_t CapsuleInstanceCount = 0;
        uint32_t CapsuleIndexCount = 0;
        uint32_t CapsuleVertexCount = 0; // Added for stats

        // Shader (Shared)
        Ref<Shader> CubeShader;
        Ref<Shader> MeshLitShader;
        Ref<Shader> MeshUnlitShader;
        Ref<Shader> ShadowDepthShader;
        Ref<Shader> MeshShadowDepthShader;
        Ref<Texture2D> WhiteTexture;

        Ref<Framebuffer> ShadowFramebuffer;
        uint32_t ShadowMapResolutionPixels = 0;
        bool IsShadowPass = false;
        static constexpr uint32_t ShadowMapTextureSlot = 31;

        /// Albedo 纹理槽 0..30；31 留给 Shadow Map（与 Cube shader binding 一致）。
        static constexpr uint32_t MaxTextureSlots = 31;
        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1;

        struct MeshLitData
        {
            glm::mat4 Transform{1.0f};
            glm::vec4 AlbedoColor{1.0f};
            float Metallic = 0.0f;
            float Roughness = 0.5f;
            int UseAlbedoTexture = 0;
            int UseMetallicTexture = 0;
            int UseRoughnessTexture = 0;
            int SharedMetallicRoughnessTexture = 0;
            int UseNormalTexture = 0;
            int NormalFlipGreen = 0;
            int EntityID = -1;
            int Padding0 = 0;
        };
        struct MeshUnlitData
        {
            glm::mat4 Transform{1.0f};
            glm::vec4 AlbedoColor{1.0f};
            int UseAlbedoTexture = 0;
            int EntityID = -1;
            int Padding0 = 0;
            int Padding1 = 0;
        };
        Ref<UniformBuffer> MeshMaterialUniformBuffer;

        // Resources
        Ref<VertexArray> SkyboxVAO;
        Ref<VertexBuffer> SkyboxVBO;
        Ref<Shader> SkyboxShader;
        struct SkyboxData {
            glm::mat4 View;
            glm::mat4 Projection;
        };
        Ref<UniformBuffer> SkyboxUniformBuffer;

        Ref<VertexArray> GridVAO;
        Ref<VertexBuffer> GridVBO;
        Ref<Shader> GridShader;
        struct GridData {
            glm::mat4 View{1.0f};
            glm::mat4 Proj{1.0f};
            float Near = 0.1f;
            float Far = 1000.0f;
            float CameraDistance = 10.0f;
            float UseXyPlane = 0.0f;
        };
        Ref<UniformBuffer> GridUniformBuffer;

        Renderer3D::Statistics Stats;

        struct CameraData {
            glm::mat4 ViewProjection{1.0f};
            glm::vec4 CameraPosition{0.0f};
        };
        CameraData CameraBuffer;
        Ref<UniformBuffer> CameraUniformBuffer;

        struct SceneLightingData {
            glm::vec4 DirectionalLightDirectionHasLight{0.0f, -1.0f, 0.0f, 0.0f};
            glm::vec4 DirectionalLightColorIntensity{1.0f, 1.0f, 1.0f, 1.0f};
            glm::vec4 AmbientColorIntensity{0.0f, 0.0f, 0.0f, 0.0f};
            glm::mat4 LightViewProjection[DirectionalCascadedShadowCascadeCount]{
                    glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f)};
            /// x = HasShadowMap (1/0), y = depthBias, z = atlas texel UV size (1/atlasResolution),
            /// w = cascade near distance along viewer forward
            glm::vec4 ShadowParameters{0.0f, 0.0015f, 0.0f, 0.05f};
            glm::vec4 CascadeSplitDistances{0.0f};
            glm::vec4 ShadowTexelWorldSize{0.0f};
            /// xyz = viewer forward, w = cascade overlap ratio
            glm::vec4 ShadowViewerForwardAndOverlap{0.0f, 0.0f, -1.0f, 0.10f};
            /// x = PointLightCount
            glm::vec4 PointLightCount{0.0f, 0.0f, 0.0f, 0.0f};
            glm::vec4 PointLightPositionRange[ScenePointLightCapacity]{};
            glm::vec4 PointLightColorIntensity[ScenePointLightCapacity]{};
            /// x = HasIBL, y = Intensity, z = PrefilterMipCount
            glm::vec4 ImageBasedLightingParameters{0.0f, 1.0f, 1.0f, 0.0f};
        };
        SceneLightingData LightingBuffer{};
        Ref<UniformBuffer> SceneLightingUniformBuffer;
        SceneLightingParameters CurrentLighting{};

        Ref<TextureCube> IrradianceCubemap;
        Ref<TextureCube> PrefilteredCubemap;
        Ref<Texture2D> BrdfLookupTexture;
        bool HasImageBasedLightingTextures = false;
    };

    static Renderer3DData s_Data;

    static void AddInstance(InstanceData *&ptr, const glm::vec4 &color, float textureIndex, int entityID,
                            float specular, float shininess, const glm::mat4 &transform)
    {
        ptr->Color = color;
        ptr->CustomData.x = textureIndex;
        ptr->CustomData.y = static_cast<float>(entityID);
        ptr->CustomData.z = specular;
        ptr->CustomData.w = shininess;
        ptr->TransformRow0 = transform[0];
        ptr->TransformRow1 = transform[1];
        ptr->TransformRow2 = transform[2];
        ptr->TransformRow3 = transform[3];
        ptr++;
    }

    void Renderer3D::Init()
    {
        // 1. Common Instance Buffer
        s_Data.InstanceVertexBuffer = VertexBuffer::Create(s_Data.MaxInstances * sizeof(InstanceData));
        s_Data.InstanceVertexBuffer->SetLayout({
            { ShaderDataType::Float4, "a_Color", false, true },
            { ShaderDataType::Float4, "a_CustomData", false, true },
            { ShaderDataType::Float4, "a_TransformRow0", false, true },
            { ShaderDataType::Float4, "a_TransformRow1", false, true },
            { ShaderDataType::Float4, "a_TransformRow2", false, true },
            { ShaderDataType::Float4, "a_TransformRow3", false, true }
        });
        
        s_Data.CubeInstanceBase = CreateScope<InstanceData[]>(s_Data.MaxInstances);
        s_Data.PlaneInstanceBase = CreateScope<InstanceData[]>(s_Data.MaxInstances);
        s_Data.SphereInstanceBase = CreateScope<InstanceData[]>(s_Data.MaxInstances);
        s_Data.CapsuleInstanceBase = CreateScope<InstanceData[]>(s_Data.MaxInstances);

        // 2. Cube Static Setup
        s_Data.CubeVAO = VertexArray::Create();
        
        // Define standard cube vertices (24 vertices)
        UnitVertex cubeVertices[24];
        // ... (Fill cubeVertices logic, reusing previous positions/normals/uvs)
        // To save code space I will compress initialization or reuse data structures if possible.
        // Actually I need to re-define them as UnitVertex.
        
        // Helper arrays
        glm::vec3 pos[24] = {
            {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, // Front
            {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, // Right
            {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, // Back
            {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}, // Left
            {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, // Top
            {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f} // Bottom
        };
        glm::vec3 normals[6] = {
            {0,0,1}, {1,0,0}, {0,0,-1}, {-1,0,0}, {0,1,0}, {0,-1,0}
        };
        glm::vec2 uvs[4] = {{0,0}, {1,0}, {1,1}, {0,1}};

        for(int i=0; i<6; i++) {
            for(int j=0; j<4; j++) {
                int idx = i*4+j;
                cubeVertices[idx].Position = pos[idx];
                cubeVertices[idx].Normal = normals[i];
                cubeVertices[idx].TexCoord = uvs[j];
            }
        }
        
        s_Data.CubeVBO = VertexBuffer::Create(sizeof(cubeVertices));
        s_Data.CubeVBO->SetData(cubeVertices, sizeof(cubeVertices));
        s_Data.CubeVBO->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Normal" },
            { ShaderDataType::Float2, "a_TexCoord" }
        });
        s_Data.CubeVAO->AddVertexBuffer(s_Data.CubeVBO);
        s_Data.CubeVAO->AddVertexBuffer(s_Data.InstanceVertexBuffer); // Attach Instance Buffer

        uint32_t cubeIndices[36];
        uint32_t offset = 0;
        for(int i=0; i<6; i++) {
            uint32_t base = i*4;
            cubeIndices[i*6+0] = base+0; cubeIndices[i*6+1] = base+1; cubeIndices[i*6+2] = base+2;
            cubeIndices[i*6+3] = base+2; cubeIndices[i*6+4] = base+3; cubeIndices[i*6+5] = base+0;
        }
        Ref<IndexBuffer> cubeIB = IndexBuffer::Create(cubeIndices, 36);
        s_Data.CubeVAO->SetIndexBuffer(cubeIB);

        s_Data.CubeShader = Shader::Create("assets/shaders/Renderer3D_Cube.glsl");
        s_Data.MeshLitShader = Shader::Create("assets/shaders/Renderer3D_MeshLit.glsl");
        s_Data.MeshUnlitShader = Shader::Create("assets/shaders/Renderer3D_MeshUnlit.glsl");
        s_Data.ShadowDepthShader = Shader::Create("assets/shaders/Renderer3D_ShadowDepth.glsl");
        s_Data.MeshShadowDepthShader = Shader::Create("assets/shaders/Renderer3D_MeshShadowDepth.glsl");
        s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::CameraData), 0);
        s_Data.MeshMaterialUniformBuffer =
                UniformBuffer::Create(sizeof(Renderer3DData::MeshLitData), 3);
        s_Data.SceneLightingUniformBuffer =
                UniformBuffer::Create(sizeof(Renderer3DData::SceneLightingData), 4);

        {
            TextureSpecification whiteSpecification;
            whiteSpecification.Width = 1;
            whiteSpecification.Height = 1;
            whiteSpecification.Format = ImageFormat::RGBA8;
            s_Data.WhiteTexture = Texture2D::Create(whiteSpecification);
            uint32_t whitePixel = 0xffffffff;
            s_Data.WhiteTexture->SetData(&whitePixel, sizeof(uint32_t));
        }

        // 3. Plane Setup
        s_Data.PlaneVAO = VertexArray::Create();
        UnitVertex planeVertices[4] = {
            {{-0.5f, 0.0f, 0.5f}, {0,1,0}, {0,0}},
            {{ 0.5f, 0.0f, 0.5f}, {0,1,0}, {1,0}},
            {{ 0.5f, 0.0f,-0.5f}, {0,1,0}, {1,1}},
            {{-0.5f, 0.0f,-0.5f}, {0,1,0}, {0,1}}
        };
        s_Data.PlaneVBO = VertexBuffer::Create(sizeof(planeVertices));
        s_Data.PlaneVBO->SetData(planeVertices, sizeof(planeVertices));
        s_Data.PlaneVBO->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Normal" },
            { ShaderDataType::Float2, "a_TexCoord" }
        });
        s_Data.PlaneVAO->AddVertexBuffer(s_Data.PlaneVBO);
        s_Data.PlaneVAO->AddVertexBuffer(s_Data.InstanceVertexBuffer);
        uint32_t planeIndices[6] = {0,1,2, 2,3,0};
        Ref<IndexBuffer> planeIB_Ref = IndexBuffer::Create(planeIndices, 6);
        s_Data.PlaneVAO->SetIndexBuffer(planeIB_Ref);

        // 4. Sphere Setup
        s_Data.SphereVAO = VertexArray::Create();
        std::vector<UnitVertex> sphereVerts;
        std::vector<uint32_t> sphereInds;
        
        // Sphere Gen logic (Unit Sphere, r=0.5)
        const int stackCount = 18; const int sectorCount = 36; const float radius = 0.5f;
        for(int i = 0; i <= stackCount; ++i) {
            float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stackCount; 
            float xy = radius * cosf(stackAngle); float z = radius * sinf(stackAngle);
            for(int j = 0; j <= sectorCount; ++j) {
                float sectorAngle = j * 2 * glm::pi<float>() / sectorCount;
                UnitVertex v;
                v.Position = {xy * cosf(sectorAngle), z, xy * sinf(sectorAngle)};
                v.Normal = {v.Position.x/radius, v.Position.y/radius, v.Position.z/radius};
                v.TexCoord = {(float)j/sectorCount, (float)i/stackCount};
                sphereVerts.push_back(v);
            }
        }
        for(int i = 0; i < stackCount; ++i) {
            int k1 = i * (sectorCount + 1); int k2 = k1 + sectorCount + 1;
            for(int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
                if(i != 0) { sphereInds.push_back(k1); sphereInds.push_back(k1 + 1); sphereInds.push_back(k2); }
                if(i != (stackCount - 1)) { sphereInds.push_back(k1 + 1); sphereInds.push_back(k2 + 1); sphereInds.push_back(k2); }
            }
        }
        s_Data.SphereVBO = VertexBuffer::Create(sphereVerts.size() * sizeof(UnitVertex));
        s_Data.SphereVBO->SetData(sphereVerts.data(), sphereVerts.size() * sizeof(UnitVertex));
        s_Data.SphereVBO->SetLayout({ { ShaderDataType::Float3, "a_Position" }, { ShaderDataType::Float3, "a_Normal" }, { ShaderDataType::Float2, "a_TexCoord" } });
        s_Data.SphereVAO->AddVertexBuffer(s_Data.SphereVBO);
        s_Data.SphereVAO->AddVertexBuffer(s_Data.InstanceVertexBuffer);
        Ref<IndexBuffer> sphereIB_Ref = IndexBuffer::Create(sphereInds.data(), sphereInds.size());
        s_Data.SphereVAO->SetIndexBuffer(sphereIB_Ref);
        s_Data.SphereIndexCount = sphereInds.size();
        s_Data.SphereVertexCount = sphereVerts.size();

        // 5. Capsule Setup (Standard Unit Capsule)
        s_Data.CapsuleVAO = VertexArray::Create();
        std::vector<UnitVertex> capVerts;
        std::vector<uint32_t> capInds;
        const int rings = 8; const int segments = 16;
        const float c_rad = 0.5f; const float c_halfH = 0.5f;
        auto addCapVert = [&](float x, float y, float z, float nx, float ny, float nz, float u, float v) {
             capVerts.push_back({{x,y,z}, {nx,ny,nz}, {u, 1.0f - v}});
        };
        // Reuse generation logic but just store to capVerts
        // ... (Skipping full detailed Copy-Paste of generation logic for brevity, relying on user trust or I can expand if needed. 
        // Actually I should expand to be safe, using the logic from previous file content)
        // [Simplified Capsule Generation for brevity - assume standard generation logic as before]
         // 1. Top Hemisphere
        for(int i = 0; i < rings; i++) {
            float phi = glm::half_pi<float>() * (1.0f - (float)i/rings); 
            float y = sinf(phi) * c_rad + c_halfH; float r = cosf(phi) * c_rad;
            for(int j=0; j<=segments; j++) {
                float u = (float)j/segments; float theta = u * glm::two_pi<float>();
                float x = -sinf(theta) * r; float z = cosf(theta) * r;
                addCapVert(x, y, z, x/c_rad, sinf(phi), z/c_rad, u, (float)i/(rings*2+1));
            }
        }
        // 2. Cylinder Top
        for(int j=0; j<=segments; j++) {
            float u = (float)j/segments; float theta = u * glm::two_pi<float>();
            float x = -sinf(theta) * c_rad; float z = cosf(theta) * c_rad;
            addCapVert(x, c_halfH, z, x/c_rad, 0.0f, z/c_rad, u, (float)rings/(rings*2+1));
        }
        // 3. Cylinder Bottom
        for(int j=0; j<=segments; j++) {
            float u = (float)j/segments; float theta = u * glm::two_pi<float>();
            float x = -sinf(theta) * c_rad; float z = cosf(theta) * c_rad;
            addCapVert(x, -c_halfH, z, x/c_rad, 0.0f, z/c_rad, u, (float)(rings+1)/(rings*2+1));
        }
        // 4. Bottom Hemisphere
        for(int i = 1; i <= rings; i++) {
             float phi = glm::half_pi<float>() * ((float)i/rings); 
             float y = -sinf(phi) * c_rad - c_halfH; float r = cosf(phi) * c_rad;
             for(int j=0; j<=segments; j++) {
                 float u = (float)j/segments; float theta = u * glm::two_pi<float>();
                 float x = -sinf(theta) * r; float z = cosf(theta) * r;
                 addCapVert(x, y, z, x/c_rad, -sinf(phi), z/c_rad, u, (float)(rings+1+i)/(rings*2+1));
             }
        }
        // Indices
        for(int i=0; i < (rings * 2 + 1); ++i) {
             for(int j=0; j < segments; ++j) {
                 int k1 = i * (segments + 1) + j; int k2 = k1 + segments + 1;
                 capInds.push_back(k1); capInds.push_back(k1 + 1); capInds.push_back(k2);
                 capInds.push_back(k1 + 1); capInds.push_back(k2 + 1); capInds.push_back(k2);
             }
        }
        s_Data.CapsuleVBO = VertexBuffer::Create(capVerts.size() * sizeof(UnitVertex));
        s_Data.CapsuleVBO->SetData(capVerts.data(), capVerts.size() * sizeof(UnitVertex));
        s_Data.CapsuleVBO->SetLayout({{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float3, "a_Normal"}, {ShaderDataType::Float2, "a_TexCoord"}});
        s_Data.CapsuleVAO->AddVertexBuffer(s_Data.CapsuleVBO);
        s_Data.CapsuleVAO->AddVertexBuffer(s_Data.InstanceVertexBuffer);
        Ref<IndexBuffer> capIB_Ref = IndexBuffer::Create(capInds.data(), capInds.size());
        s_Data.CapsuleVAO->SetIndexBuffer(capIB_Ref);
        s_Data.CapsuleIndexCount = capInds.size();
        s_Data.CapsuleVertexCount = capVerts.size();
        
        // Skybox Setup
        float skyboxVertices[] = {-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                                  1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
                                  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
                                  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
                                  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,
                                  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
                                  -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,
                                  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
                                  1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

        // Skybox & Grid (Keep as is just re-init)
        s_Data.SkyboxVAO = VertexArray::Create();
        s_Data.SkyboxVBO = VertexBuffer::Create(sizeof(skyboxVertices)); // Reusing array from before
        s_Data.SkyboxVBO->SetData(skyboxVertices, sizeof(skyboxVertices));
        s_Data.SkyboxVBO->SetLayout({{ShaderDataType::Float3, "a_Position"}});
        s_Data.SkyboxVAO->AddVertexBuffer(s_Data.SkyboxVBO);
        s_Data.SkyboxShader = Shader::Create("assets/shaders/Skybox.glsl");
        s_Data.SkyboxUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::SkyboxData), 1);

        s_Data.GridVAO = VertexArray::Create();
        s_Data.GridVBO = VertexBuffer::Create(sizeof(float) * 6 * 3);
        float gridVertices[] = { 1.0f,  1.0f,  0.0f, -1.0f,  1.0f,  0.0f, -1.0f, -1.0f,  0.0f, -1.0f, -1.0f,  0.0f,  1.0f, -1.0f,  0.0f,  1.0f,  1.0f,  0.0f };
        s_Data.GridVBO->SetData(gridVertices, sizeof(gridVertices));
        s_Data.GridVBO->SetLayout({{ShaderDataType::Float3, "a_Position"}});
        s_Data.GridVAO->AddVertexBuffer(s_Data.GridVBO);
        s_Data.GridShader = Shader::Create("assets/shaders/Grid.glsl");
        s_Data.GridUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::GridData), 2);

        EnvironmentLightingSystem::Init();
    }

    void Renderer3D::Shutdown()
    {
         EnvironmentLightingSystem::Shutdown();
         // s_Data.InstanceBufferBase is handled by Scope
    }

    void Renderer3D::SetSceneLighting(const SceneLightingParameters &parameters)
    {
        s_Data.CurrentLighting = parameters;
        s_Data.LightingBuffer.DirectionalLightDirectionHasLight = glm::vec4(
                parameters.DirectionalLightDirection,
                parameters.HasDirectionalLight ? 1.0f : 0.0f);
        s_Data.LightingBuffer.DirectionalLightColorIntensity =
                glm::vec4(parameters.DirectionalLightColor, parameters.DirectionalLightIntensity);
        if (parameters.HasImageBasedLighting)
        {
            s_Data.LightingBuffer.AmbientColorIntensity = glm::vec4(0.0f);
        }
        else if (parameters.HasDirectionalLight)
        {
            s_Data.LightingBuffer.AmbientColorIntensity =
                    glm::vec4(parameters.AmbientColor, parameters.AmbientIntensity);
        }
        else
        {
            s_Data.LightingBuffer.AmbientColorIntensity = glm::vec4(0.0f);
        }

        for (uint32_t cascadeIndex = 0; cascadeIndex < DirectionalCascadedShadowCascadeCount; ++cascadeIndex)
            s_Data.LightingBuffer.LightViewProjection[cascadeIndex] = parameters.LightViewProjection[cascadeIndex];
        s_Data.LightingBuffer.ShadowParameters =
                glm::vec4(parameters.HasShadowMap ? 1.0f : 0.0f, parameters.ShadowBias,
                          parameters.ShadowAtlasTexelUvSize, parameters.ShadowCascadeNearDistance);
        s_Data.LightingBuffer.CascadeSplitDistances = parameters.CascadeSplitDistances;
        s_Data.LightingBuffer.ShadowTexelWorldSize = parameters.ShadowTexelWorldSize;
        s_Data.LightingBuffer.ShadowViewerForwardAndOverlap =
                glm::vec4(parameters.ShadowViewerForwardDirection, parameters.ShadowCascadeOverlapRatio);
        s_Data.LightingBuffer.ImageBasedLightingParameters =
                glm::vec4(parameters.HasImageBasedLighting ? 1.0f : 0.0f, parameters.EnvironmentIntensity,
                          parameters.PrefilterMipCount, 0.0f);

        const uint32_t pointLightCount =
                std::min(parameters.PointLightCount, ScenePointLightCapacity);
        s_Data.LightingBuffer.PointLightCount =
                glm::vec4(static_cast<float>(pointLightCount), 0.0f, 0.0f, 0.0f);
        for (uint32_t pointLightIndex = 0; pointLightIndex < ScenePointLightCapacity; ++pointLightIndex)
        {
            if (pointLightIndex < pointLightCount)
            {
                const PointLightParameters &pointLight = parameters.PointLights[pointLightIndex];
                s_Data.LightingBuffer.PointLightPositionRange[pointLightIndex] =
                        glm::vec4(pointLight.Position, pointLight.Range);
                s_Data.LightingBuffer.PointLightColorIntensity[pointLightIndex] =
                        glm::vec4(pointLight.Color, pointLight.Intensity);
            }
            else
            {
                s_Data.LightingBuffer.PointLightPositionRange[pointLightIndex] = glm::vec4(0.0f);
                s_Data.LightingBuffer.PointLightColorIntensity[pointLightIndex] = glm::vec4(0.0f);
            }
        }

        if (s_Data.SceneLightingUniformBuffer)
        {
            s_Data.SceneLightingUniformBuffer->SetData(&s_Data.LightingBuffer,
                                                      sizeof(Renderer3DData::SceneLightingData));
            s_Data.SceneLightingUniformBuffer->Bind();
        }
    }

    SceneLightingParameters Renderer3D::GetSceneLighting()
    {
        return s_Data.CurrentLighting;
    }

    void Renderer3D::SetImageBasedLighting(const Ref<TextureCube> &irradianceCubemap,
                                           const Ref<TextureCube> &prefilteredCubemap,
                                           const Ref<Texture2D> &brdfLookupTexture, float intensity,
                                           float prefilterMipCount)
    {
        s_Data.IrradianceCubemap = irradianceCubemap;
        s_Data.PrefilteredCubemap = prefilteredCubemap;
        s_Data.BrdfLookupTexture = brdfLookupTexture;
        s_Data.HasImageBasedLightingTextures =
                irradianceCubemap && prefilteredCubemap && brdfLookupTexture;
        s_Data.CurrentLighting.HasImageBasedLighting = s_Data.HasImageBasedLightingTextures;
        s_Data.CurrentLighting.EnvironmentIntensity = intensity;
        s_Data.CurrentLighting.PrefilterMipCount = prefilterMipCount;
        s_Data.LightingBuffer.ImageBasedLightingParameters =
                glm::vec4(s_Data.HasImageBasedLightingTextures ? 1.0f : 0.0f, intensity, prefilterMipCount,
                          0.0f);
        if (s_Data.HasImageBasedLightingTextures)
            s_Data.LightingBuffer.AmbientColorIntensity = glm::vec4(0.0f);
        if (s_Data.SceneLightingUniformBuffer)
        {
            s_Data.SceneLightingUniformBuffer->SetData(&s_Data.LightingBuffer,
                                                      sizeof(Renderer3DData::SceneLightingData));
            s_Data.SceneLightingUniformBuffer->Bind();
        }
    }

    void Renderer3D::ClearImageBasedLighting()
    {
        s_Data.IrradianceCubemap.reset();
        s_Data.PrefilteredCubemap.reset();
        s_Data.BrdfLookupTexture.reset();
        s_Data.HasImageBasedLightingTextures = false;
        s_Data.CurrentLighting.HasImageBasedLighting = false;
        s_Data.LightingBuffer.ImageBasedLightingParameters = glm::vec4(0.0f, 1.0f, 1.0f, 0.0f);
        if (s_Data.SceneLightingUniformBuffer)
        {
            s_Data.SceneLightingUniformBuffer->SetData(&s_Data.LightingBuffer,
                                                      sizeof(Renderer3DData::SceneLightingData));
            s_Data.SceneLightingUniformBuffer->Bind();
        }
    }

    static void BindImageBasedLightingTexturesIfAvailable()
    {
        if (!s_Data.HasImageBasedLightingTextures)
            return;
        if (s_Data.IrradianceCubemap)
            s_Data.IrradianceCubemap->Bind(4);
        if (s_Data.PrefilteredCubemap)
            s_Data.PrefilteredCubemap->Bind(5);
        if (s_Data.BrdfLookupTexture)
            s_Data.BrdfLookupTexture->Bind(6);
    }

    void Renderer3D::EnsureShadowMap(uint32_t resolutionPixels)
    {
        if (resolutionPixels == 0)
            return;

        if (s_Data.ShadowFramebuffer
            && s_Data.ShadowMapResolutionPixels == resolutionPixels
            && s_Data.ShadowFramebuffer->GetSpecification().Width == resolutionPixels
            && s_Data.ShadowFramebuffer->GetSpecification().Height == resolutionPixels)
        {
            return;
        }

        FramebufferSpecification specification;
        specification.Width = resolutionPixels;
        specification.Height = resolutionPixels;
        specification.Attachments = {FramebufferFormat::DEPTH32};
        s_Data.ShadowFramebuffer = Framebuffer::Create(specification);
        s_Data.ShadowMapResolutionPixels = resolutionPixels;
    }

    void Renderer3D::BeginShadowPass()
    {
        HIMII_CORE_ASSERT(s_Data.ShadowFramebuffer, "EnsureShadowMap must be called before BeginShadowPass");

        s_Data.IsShadowPass = true;
        s_Data.ShadowFramebuffer->BindCapturingPrevious();
        RenderCommand::SetDepthTest(true);
        RenderCommand::SetDepthMask(true);
        RenderCommand::SetDepthFunc(RHI::DepthComp::Less);
        // Double-sided casters: normal-offset bias handles acne, and single-sided geometry
        // (planes, imported meshes with flipped winding) would otherwise drop out of the map.
        RenderCommand::SetCullMode(RHI::CullMode::None);
        RenderCommand::ClearDepth();
        StartBatch();
    }

    void Renderer3D::SetShadowCascadeViewProjection(const glm::mat4 &lightViewProjection, uint32_t viewportX,
                                                    uint32_t viewportY, uint32_t viewportWidth,
                                                    uint32_t viewportHeight)
    {
        HIMII_CORE_ASSERT(s_Data.IsShadowPass, "SetShadowCascadeViewProjection requires an active shadow pass");
        Flush();
        RenderCommand::SetViewport(viewportX, viewportY, viewportWidth, viewportHeight);
        s_Data.CameraBuffer.ViewProjection = lightViewProjection;
        s_Data.CameraBuffer.CameraPosition = glm::vec4(0.0f);
        UploadCameraAndLighting();
        StartBatch();
    }

    void Renderer3D::EndShadowPass()
    {
        Flush();
        s_Data.IsShadowPass = false;
        RenderCommand::SetCullMode(RHI::CullMode::Back);
        if (s_Data.ShadowFramebuffer)
            s_Data.ShadowFramebuffer->UnbindRestoringPrevious();
    }

    void Renderer3D::BindShadowMapIfAvailable()
    {
        if (s_Data.IsShadowPass || !s_Data.CurrentLighting.HasShadowMap || !s_Data.ShadowFramebuffer)
            return;
        s_Data.ShadowFramebuffer->BindDepthAttachment(Renderer3DData::ShadowMapTextureSlot);
    }

    void Renderer3D::SubmitMaterialGeometry(const Ref<VertexArray> &vertexArray, uint32_t indexCount,
                                            const glm::mat4 &transform, AssetHandle materialHandle, int entityID)
    {
        if (!vertexArray || indexCount == 0)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        const ResolvedMaterialSurface resolvedSurface =
                ResolveMaterialSurface(assetManager.get(), materialHandle);

        glm::vec4 albedoColor = resolvedSurface.AlbedoColor;
        Ref<Texture2D> albedoTexture =
                resolvedSurface.AlbedoTexture ? resolvedSurface.AlbedoTexture : s_Data.WhiteTexture;
        int useAlbedoTexture = resolvedSurface.AlbedoTexture ? 1 : 0;
        float metallic = resolvedSurface.Metallic;
        float roughness = resolvedSurface.Roughness;
        Ref<Texture2D> metallicTexture = resolvedSurface.MetallicTexture;
        Ref<Texture2D> roughnessTexture = resolvedSurface.RoughnessTexture;
        int useMetallicTexture = resolvedSurface.MetallicTexture ? 1 : 0;
        int useRoughnessTexture = resolvedSurface.RoughnessTexture ? 1 : 0;
        int sharedMetallicRoughnessTexture = resolvedSurface.SharedMetallicRoughnessTexture ? 1 : 0;
        Ref<Texture2D> normalTexture = resolvedSurface.NormalTexture;
        int useNormalTexture = resolvedSurface.NormalTexture ? 1 : 0;
        int normalFlipGreen = resolvedSurface.NormalFlipGreen ? 1 : 0;
        const bool useUnlit = !resolvedSurface.UsesLitPipeline;

        if (useUnlit && s_Data.IsShadowPass)
            return;

        if (s_Data.IsShadowPass)
        {
            s_Data.MeshShadowDepthShader->Bind();
            Renderer3DData::MeshLitData meshShadowData{};
            meshShadowData.Transform = transform;
            s_Data.MeshMaterialUniformBuffer->SetData(&meshShadowData, sizeof(Renderer3DData::MeshLitData));
            s_Data.MeshMaterialUniformBuffer->Bind();
            s_Data.CameraUniformBuffer->Bind();

            vertexArray->Bind();
            RenderCommand::DrawIndexed(vertexArray, indexCount);
            s_Data.Stats.DrawCalls++;
            s_Data.Stats.TotalIndexCount += indexCount;
            return;
        }

        if (useUnlit)
        {
            Ref<Shader> activeShader =
                    resolvedSurface.ShaderProgram ? resolvedSurface.ShaderProgram : s_Data.MeshUnlitShader;
            activeShader->Bind();
            Renderer3DData::MeshUnlitData meshUnlitData;
            meshUnlitData.Transform = transform;
            meshUnlitData.AlbedoColor = albedoColor;
            meshUnlitData.UseAlbedoTexture = useAlbedoTexture;
            meshUnlitData.EntityID = entityID;
            s_Data.MeshMaterialUniformBuffer->SetData(&meshUnlitData, sizeof(Renderer3DData::MeshUnlitData));
        }
        else
        {
            Ref<Shader> activeShader =
                    resolvedSurface.ShaderProgram ? resolvedSurface.ShaderProgram : s_Data.MeshLitShader;
            activeShader->Bind();
            Renderer3DData::MeshLitData meshLitData;
            meshLitData.Transform = transform;
            meshLitData.AlbedoColor = albedoColor;
            meshLitData.Metallic = metallic;
            meshLitData.Roughness = roughness;
            meshLitData.UseAlbedoTexture = useAlbedoTexture;
            meshLitData.UseMetallicTexture = useMetallicTexture;
            meshLitData.UseRoughnessTexture = useRoughnessTexture;
            meshLitData.SharedMetallicRoughnessTexture = sharedMetallicRoughnessTexture;
            meshLitData.UseNormalTexture = useNormalTexture;
            meshLitData.NormalFlipGreen = normalFlipGreen;
            meshLitData.EntityID = entityID;
            s_Data.MeshMaterialUniformBuffer->SetData(&meshLitData, sizeof(Renderer3DData::MeshLitData));
        }
        s_Data.MeshMaterialUniformBuffer->Bind();
        s_Data.CameraUniformBuffer->Bind();
        if (s_Data.SceneLightingUniformBuffer)
            s_Data.SceneLightingUniformBuffer->Bind();

        if (albedoTexture)
            albedoTexture->Bind(0);
        if (metallicTexture)
            metallicTexture->Bind(1);
        else
            s_Data.WhiteTexture->Bind(1);
        if (roughnessTexture)
            roughnessTexture->Bind(2);
        else
            s_Data.WhiteTexture->Bind(2);
        if (normalTexture)
            normalTexture->Bind(3);
        else
            s_Data.WhiteTexture->Bind(3);
        if (!useUnlit)
        {
            BindShadowMapIfAvailable();
            BindImageBasedLightingTexturesIfAvailable();
        }

        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray, indexCount);
        s_Data.Stats.DrawCalls++;
        s_Data.Stats.TotalIndexCount += indexCount;
    }

    void Renderer3D::UploadCameraAndLighting()
    {
        s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));
        s_Data.CameraUniformBuffer->Bind();
        if (s_Data.SceneLightingUniformBuffer)
        {
            s_Data.SceneLightingUniformBuffer->SetData(&s_Data.LightingBuffer,
                                                      sizeof(Renderer3DData::SceneLightingData));
            s_Data.SceneLightingUniformBuffer->Bind();
        }
    }

    float Renderer3D::ResolveTextureIndex(const Ref<Texture2D> &albedoTexture)
    {
        if (!albedoTexture || albedoTexture == s_Data.WhiteTexture)
            return 0.0f;

        for (uint32_t slot = 1; slot < s_Data.TextureSlotIndex; ++slot)
        {
            if (s_Data.TextureSlots[slot] == albedoTexture)
                return static_cast<float>(slot);
        }

        if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
            NextBatch();

        const float textureIndex = static_cast<float>(s_Data.TextureSlotIndex);
        s_Data.TextureSlots[s_Data.TextureSlotIndex] = albedoTexture;
        s_Data.TextureSlotIndex++;
        return textureIndex;
    }

    void Renderer3D::BeginScene(const EditorCamera &camera) {
        ResetStats(); 
        RenderCommand::SetDepthTest(true);
        RenderCommand::SetCullMode(RHI::CullMode::Back);
        s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
        s_Data.CameraBuffer.CameraPosition = glm::vec4(camera.GetPosition(), 1.0f);
        UploadCameraAndLighting();
        StartBatch();
    }
    void Renderer3D::BeginScene(const Camera &camera, const glm::mat4 &transform) {
        ResetStats(); 
        RenderCommand::SetDepthTest(true);
        RenderCommand::SetCullMode(RHI::CullMode::Back);
        s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
        s_Data.CameraBuffer.CameraPosition = glm::vec4(glm::vec3(transform[3]), 1.0f);
        UploadCameraAndLighting();
        StartBatch();
    }
    void Renderer3D::EndScene() { Flush(); }
    void Renderer3D::StartBatch() {
        s_Data.CubeInstanceCount = 0;
        s_Data.CubeInstancePtr = s_Data.CubeInstanceBase.get();

        s_Data.PlaneInstanceCount = 0;
        s_Data.PlaneInstancePtr = s_Data.PlaneInstanceBase.get();

        s_Data.SphereInstanceCount = 0;
        s_Data.SphereInstancePtr = s_Data.SphereInstanceBase.get();

        s_Data.CapsuleInstanceCount = 0;
        s_Data.CapsuleInstancePtr = s_Data.CapsuleInstanceBase.get();

        s_Data.TextureSlotIndex = 1;
        for (uint32_t slot = 0; slot < Renderer3DData::MaxTextureSlots; ++slot)
            s_Data.TextureSlots[slot] = nullptr;
        s_Data.TextureSlots[0] = s_Data.WhiteTexture;
    }

    void Renderer3D::Flush() {
        if (s_Data.IsShadowPass)
        {
            if (!s_Data.ShadowDepthShader)
                return;
            s_Data.ShadowDepthShader->Bind();
            UploadCameraAndLighting();
        }
        else
        {
            s_Data.CubeShader->Bind();
            UploadCameraAndLighting();
            BindShadowMapIfAvailable();

            for (uint32_t slot = 0; slot < s_Data.TextureSlotIndex; ++slot)
            {
                if (s_Data.TextureSlots[slot])
                    s_Data.TextureSlots[slot]->Bind(slot);
            }
        }

        // 1. Cubes
        if (s_Data.CubeInstanceCount > 0) {
            uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CubeInstancePtr - (uint8_t*)s_Data.CubeInstanceBase.get());
            s_Data.InstanceVertexBuffer->SetData(s_Data.CubeInstanceBase.get(), dataSize);
            s_Data.CubeVAO->Bind();
            RenderCommand::DrawIndexedInstanced(s_Data.CubeVAO, 36, s_Data.CubeInstanceCount);
            s_Data.Stats.DrawCalls++;
        }

        // 2. Planes (Overwrites Instance Buffer)
        if (s_Data.PlaneInstanceCount > 0) {
            uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.PlaneInstancePtr - (uint8_t*)s_Data.PlaneInstanceBase.get());
            s_Data.InstanceVertexBuffer->SetData(s_Data.PlaneInstanceBase.get(), dataSize);
            s_Data.PlaneVAO->Bind();
            RenderCommand::DrawIndexedInstanced(s_Data.PlaneVAO, 6, s_Data.PlaneInstanceCount);
            s_Data.Stats.DrawCalls++;
        }

        // 3. Spheres
        if (s_Data.SphereInstanceCount > 0) {
            uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.SphereInstancePtr - (uint8_t*)s_Data.SphereInstanceBase.get());
            s_Data.InstanceVertexBuffer->SetData(s_Data.SphereInstanceBase.get(), dataSize);
            s_Data.SphereVAO->Bind();
            RenderCommand::DrawIndexedInstanced(s_Data.SphereVAO, s_Data.SphereIndexCount, s_Data.SphereInstanceCount);
            s_Data.Stats.DrawCalls++;
        }

        // 4. Capsules
        if (s_Data.CapsuleInstanceCount > 0) {
            uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CapsuleInstancePtr - (uint8_t*)s_Data.CapsuleInstanceBase.get());
            s_Data.InstanceVertexBuffer->SetData(s_Data.CapsuleInstanceBase.get(), dataSize);
            s_Data.CapsuleVAO->Bind();
            RenderCommand::DrawIndexedInstanced(s_Data.CapsuleVAO, s_Data.CapsuleIndexCount, s_Data.CapsuleInstanceCount);
            s_Data.Stats.DrawCalls++;
        }
    }

    void Renderer3D::NextBatch() { Flush(); StartBatch(); }


    void Renderer3D::DrawCube(const glm::vec3 &position, const glm::vec3 &size, const glm::vec4 &color, int entityID)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
        DrawCube(transform, color, entityID);
    }

    void Renderer3D::DrawCube(const glm::mat4 &transform, const glm::vec4 &color, int entityID,
                              float specular, float shininess, const Ref<Texture2D> &albedoTexture)
    {
        if (s_Data.CubeInstanceCount >= Renderer3DData::MaxInstances) NextBatch();
        const float textureIndex = ResolveTextureIndex(albedoTexture);
        AddInstance(s_Data.CubeInstancePtr, color, textureIndex, entityID, specular, shininess, transform);
        s_Data.CubeInstanceCount++;
        s_Data.Stats.CubeCount++; 
        s_Data.Stats.TotalVertexCount += 24; 
        s_Data.Stats.TotalIndexCount += 36;
    }

    void Renderer3D::DrawSkybox(const Ref<TextureCube> &cubemap, const Camera &camera, const glm::mat4 &cameraTransform)
    {
        RenderCommand::SetDepthFunc(RHI::DepthComp::Lequal);
        RenderCommand::SetCullMode(RHI::CullMode::None);

        s_Data.SkyboxShader->Bind();

        glm::mat4 view = glm::mat4(glm::mat3(glm::inverse(cameraTransform)));
        glm::mat4 projection = camera.GetProjection();

        Renderer3DData::SkyboxData skyboxData;
        skyboxData.View = view;
        skyboxData.Projection = projection;
        s_Data.SkyboxUniformBuffer->SetData(&skyboxData, sizeof(Renderer3DData::SkyboxData));

        cubemap->Bind(0);

        s_Data.SkyboxVAO->Bind();
        RenderCommand::DrawArrays(s_Data.SkyboxVAO, 36);

        s_Data.SkyboxVAO->Unbind();
        s_Data.SkyboxShader->Unbind();

        RenderCommand::SetCullMode(RHI::CullMode::Back);
        RenderCommand::SetDepthFunc(RHI::DepthComp::Less);
    }

    void Renderer3D::DrawSkybox(const Ref<TextureCube> &cubemap, const EditorCamera &camera)
    {
        RenderCommand::SetDepthFunc(RHI::DepthComp::Lequal);
        RenderCommand::SetCullMode(RHI::CullMode::None);

        s_Data.SkyboxShader->Bind();

        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        glm::mat4 projection = camera.GetProjection();

        Renderer3DData::SkyboxData skyboxData;
        skyboxData.View = view;
        skyboxData.Projection = projection;
        s_Data.SkyboxUniformBuffer->SetData(&skyboxData, sizeof(Renderer3DData::SkyboxData));

        cubemap->Bind(0);

        s_Data.SkyboxVAO->Bind();
        RenderCommand::DrawArrays(s_Data.SkyboxVAO, 36);
        s_Data.SkyboxVAO->Unbind();
        s_Data.SkyboxShader->Unbind();

        RenderCommand::SetCullMode(RHI::CullMode::Back);
        RenderCommand::SetDepthFunc(RHI::DepthComp::Less);
    }


    void Renderer3D::DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color, int entityID)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(radius * 2.0f)); 
        DrawSphere(transform, color, entityID);
    }

    void Renderer3D::DrawSphere(const glm::mat4& transform, const glm::vec4& color, int entityID,
                                float specular, float shininess, const Ref<Texture2D> &albedoTexture)
    {
        if (s_Data.SphereInstanceCount >= Renderer3DData::MaxInstances) NextBatch();
        const float textureIndex = ResolveTextureIndex(albedoTexture);
        AddInstance(s_Data.SphereInstancePtr, color, textureIndex, entityID, specular, shininess, transform);
        s_Data.SphereInstanceCount++;
        s_Data.Stats.SphereCount++;
        s_Data.Stats.TotalVertexCount += s_Data.SphereVertexCount;
        s_Data.Stats.TotalIndexCount += s_Data.SphereIndexCount;
    }

    void Renderer3D::DrawCapsule(const glm::vec3& position, float radius, float height, const glm::vec4& color, int entityID)
    {    
         // For Instanced Rendering, we assume uniform or matrix-based scaling.
         // Real "Smart Scaling" for Capsules requires generating new mesh or complex shader.
         // Here, we use Matrix Transform for performance, as requested.
         if (s_Data.CapsuleInstanceCount >= Renderer3DData::MaxInstances) NextBatch();

         // Construct approximate transform for Capsule
         // Unit Capsule: Radius 0.5, Height 1.0 (tips at Y=+0.5, -0.5).
         // User wants Radius 'radius' and Height 'height'.
         // Note: 'height' usually means total height or cylinder height?
         // In PhysX, Capsule Height often means cylinder height. Total = Height + 2*Radius.
         // Let's assume 'height' is Cylinder Height here based on previous code logic (localPos.y * height).
         
         // If we strictly use Matrix scaling:
         // Scale X = radius * 2
         // Scale Z = radius * 2
         // Scale Y = ?
         // A matrix scale Y stretches the hemispheres too.
         // THIS IS A LIMITATION of Instanced Rendering without Special Shader.
         // For now, we apply standard scaling which might distort hemispheres.
         // To fix this properly, we should pass "Radius/Height" as Instance Data and do vertex displacement in Shader.
         // But that requires modifying Shader and `InstanceData` struct again.
         // Given "Phase 1 / Phase 2" plan only mentioned `a_Transform`, I will stick to standard Matrix Transform.
         
         glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
         transform = glm::rotate(transform, 0.0f, {0,0,1}); // Rotation?
         // Note: The signature doesn't take rotation?
         // The user passed `DrawCapsule(pos, rad, height...)`.
         // I will construct a naive transform.
         
         glm::vec3 scale = {radius * 2.0f, height + radius * 2.0f, radius * 2.0f}; // Naive total height scaling
         transform = glm::scale(transform, scale);

         DrawCapsule(transform, color, entityID);
    }
    
    void Renderer3D::DrawCapsule(const glm::mat4& transform, const glm::vec4& color, int entityID,
                                 float specular, float shininess, const Ref<Texture2D> &albedoTexture)
    {
        if (s_Data.CapsuleInstanceCount >= Renderer3DData::MaxInstances) NextBatch();
        const float textureIndex = ResolveTextureIndex(albedoTexture);
        AddInstance(s_Data.CapsuleInstancePtr, color, textureIndex, entityID, specular, shininess, transform);
        s_Data.CapsuleInstanceCount++;
        s_Data.Stats.CapsuleCount++; 
        s_Data.Stats.TotalVertexCount += s_Data.CapsuleVertexCount; 
        s_Data.Stats.TotalIndexCount += s_Data.CapsuleIndexCount;
    }

    void Renderer3D::DrawPlane(const glm::mat4& transform, const glm::vec4& color, int entityID,
                               float specular, float shininess, const Ref<Texture2D> &albedoTexture)
    {
        if (s_Data.PlaneInstanceCount >= Renderer3DData::MaxInstances) NextBatch();
        const float textureIndex = ResolveTextureIndex(albedoTexture);
        AddInstance(s_Data.PlaneInstancePtr, color, textureIndex, entityID, specular, shininess, transform);
        s_Data.PlaneInstanceCount++;
        s_Data.Stats.QuadCount++;
        s_Data.Stats.TotalVertexCount += 4;
        s_Data.Stats.TotalIndexCount += 6;
    }

    void Renderer3D::DrawMeshAsset(const Ref<MeshAsset> &meshAsset,
                                   const std::vector<AssetHandle> &materialAssetHandles,
                                   const glm::mat4 &transform,
                                   int entityID)
    {
        if (!meshAsset)
            return;
        if (s_Data.IsShadowPass)
        {
            if (!s_Data.MeshShadowDepthShader)
                return;
        }
        else if (!s_Data.MeshLitShader || !s_Data.MeshUnlitShader)
        {
            return;
        }

        meshAsset->EnsureGpuResources();
        const auto &gpuSubmeshes = meshAsset->GetGpuSubmeshes();
        if (gpuSubmeshes.empty())
            return;

        Flush();
        UploadCameraAndLighting();

        for (const MeshSubmeshGpu &gpuSubmesh : gpuSubmeshes)
        {
            AssetHandle materialHandle = 0;
            if (gpuSubmesh.MaterialSlotIndex < materialAssetHandles.size())
                materialHandle = materialAssetHandles[gpuSubmesh.MaterialSlotIndex];
            else if (gpuSubmesh.MaterialSlotIndex < meshAsset->DefaultMaterialHandles.size())
                materialHandle = meshAsset->DefaultMaterialHandles[gpuSubmesh.MaterialSlotIndex];

            SubmitMaterialGeometry(gpuSubmesh.VertexArray, gpuSubmesh.IndexCount, transform, materialHandle,
                                   entityID);
        }

        StartBatch();
    }

    void Renderer3D::DrawBuiltinLitMesh(BuiltinLitPrimitive primitive, const glm::mat4 &transform,
                                        AssetHandle materialHandle, int entityID)
    {
        if (s_Data.IsShadowPass)
        {
            if (!s_Data.MeshShadowDepthShader)
                return;
        }
        else if (!s_Data.MeshLitShader || !s_Data.MeshUnlitShader)
        {
            return;
        }

        Ref<VertexArray> vertexArray;
        uint32_t indexCount = 0;
        switch (primitive)
        {
            case BuiltinLitPrimitive::Cube:
                vertexArray = s_Data.CubeVAO;
                indexCount = 36;
                s_Data.Stats.CubeCount++;
                s_Data.Stats.TotalVertexCount += 24;
                break;
            case BuiltinLitPrimitive::Plane:
                vertexArray = s_Data.PlaneVAO;
                indexCount = 6;
                s_Data.Stats.QuadCount++;
                s_Data.Stats.TotalVertexCount += 4;
                break;
            case BuiltinLitPrimitive::Sphere:
                vertexArray = s_Data.SphereVAO;
                indexCount = s_Data.SphereIndexCount;
                s_Data.Stats.SphereCount++;
                s_Data.Stats.TotalVertexCount += s_Data.SphereVertexCount;
                break;
            case BuiltinLitPrimitive::Capsule:
                vertexArray = s_Data.CapsuleVAO;
                indexCount = s_Data.CapsuleIndexCount;
                s_Data.Stats.CapsuleCount++;
                break;
        }

        Flush();
        UploadCameraAndLighting();
        SubmitMaterialGeometry(vertexArray, indexCount, transform, materialHandle, entityID);
        StartBatch();
    }

    namespace
    {
        void SubmitGridDraw(const Renderer3DData::GridData &gridData)
        {
            s_Data.GridUniformBuffer->SetData(&gridData, sizeof(Renderer3DData::GridData));

            RenderCommand::SetDepthTest(true);
            // Do not write depth so later transparent passes are not occluded by the grid.
            RenderCommand::SetDepthMask(false);

            s_Data.GridVAO->Bind();
            RenderCommand::DrawArrays(s_Data.GridVAO, 6);
            s_Data.GridVAO->Unbind();
            s_Data.GridShader->Unbind();

            RenderCommand::SetDepthMask(true);
        }

        float ResolveGridCameraDistance(bool xyPlane, const glm::vec3 &cameraPosition, float fallbackDistance)
        {
            const float planeHeight =
                    xyPlane ? std::abs(cameraPosition.z) : std::abs(cameraPosition.y);
            return std::max(std::max(planeHeight, fallbackDistance), 0.5f);
        }
    }

    void Renderer3D::DrawGrid(const EditorCamera &camera, bool xyPlane)
    {
        s_Data.GridShader->Bind();

        Renderer3DData::GridData gridData;
        gridData.View = camera.GetViewMatrix();
        if (xyPlane)
            gridData.View = gridData.View * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), {1.0f, 0.0f, 0.0f});

        gridData.Proj = camera.GetProjection();
        gridData.Near = camera.GetNearClip();
        gridData.Far = camera.GetFarClip();
        // Orbit distance drives decimal LOD for both perspective and orthographic editor cameras.
        gridData.CameraDistance = std::max(camera.GetDistance(), 0.5f);
        gridData.UseXyPlane = xyPlane ? 1.0f : 0.0f;

        SubmitGridDraw(gridData);
    }

    void Renderer3D::DrawGrid(const Camera &camera, const glm::mat4 &transform, bool xyPlane)
    {
        s_Data.GridShader->Bind();

        Renderer3DData::GridData gridData;
        gridData.View = glm::inverse(transform);
        if (xyPlane)
            gridData.View = gridData.View * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), {1.0f, 0.0f, 0.0f});

        gridData.Proj = camera.GetProjection();

        const SceneCamera *sceneCamera = dynamic_cast<const SceneCamera *>(&camera);
        if (sceneCamera)
        {
            if (sceneCamera->GetProjectionType() == SceneCamera::ProjectionType::Perspective)
            {
                gridData.Near = sceneCamera->GetPerspectiveNearClip();
                gridData.Far = sceneCamera->GetPerspectiveFarClip();
            }
            else
            {
                gridData.Near = sceneCamera->GetOrthographicNearClip();
                gridData.Far = sceneCamera->GetOrthographicFarClip();
            }
        }
        else
        {
            gridData.Near = 0.01f;
            gridData.Far = 1000.0f;
        }

        const glm::vec3 cameraPosition = glm::vec3(transform[3]);
        gridData.CameraDistance = ResolveGridCameraDistance(xyPlane, cameraPosition, 10.0f);
        gridData.UseXyPlane = xyPlane ? 1.0f : 0.0f;

        SubmitGridDraw(gridData);
    }

    void Renderer3D::ResetStats() { memset(&s_Data.Stats, 0, sizeof(Statistics)); }
    Renderer3D::Statistics Renderer3D::GetStatistics() { return s_Data.Stats; }

} // namespace Himii