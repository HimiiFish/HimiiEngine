#include "Hepch.h"
#include "Module/Render/Mesh/FbxMeshGeometryLoader.h"
#include "Module/Render/Mesh/MeshTangentUtility.h"
#include "EngineCore/Core/Log.h"

#include <unordered_map>
#include <vector>

#include "ufbx.h"

namespace Himii
{
    namespace
    {
        glm::vec3 ToVec3(const ufbx_vec3 &value)
        {
            return {value.x, value.y, value.z};
        }

        glm::vec2 ToVec2(const ufbx_vec2 &value)
        {
            return {value.x, value.y};
        }

        void AppendMeshInstance(const ufbx_node *node,
                                const std::unordered_map<const ufbx_material *, uint32_t> &materialSlotMap,
                                MeshAsset &outMeshAsset, std::vector<uint32_t> &triangleIndicesScratch)
        {
            if (!node || !node->mesh)
                return;

            const ufbx_mesh *mesh = node->mesh;
            if (!mesh->vertex_position.exists)
                return;

            const ufbx_matrix &geometryToWorld = node->geometry_to_world;
            const ufbx_matrix normalMatrix = ufbx_matrix_for_normals(&geometryToWorld);

            const size_t materialPartCount =
                    mesh->material_parts.count > 0 ? mesh->material_parts.count : 1;

            for (size_t partIndex = 0; partIndex < materialPartCount; ++partIndex)
            {
                const uint32_t indexStart = static_cast<uint32_t>(outMeshAsset.Indices.size());
                uint32_t addedIndexCount = 0;

                const bool hasMaterialParts = mesh->material_parts.count > 0;
                size_t faceCount = mesh->faces.count;
                const uint32_t *faceIndexList = nullptr;
                if (hasMaterialParts)
                {
                    const ufbx_mesh_part &part = mesh->material_parts.data[partIndex];
                    faceCount = part.num_faces;
                    faceIndexList = part.face_indices.data;
                }

                uint32_t materialSlotIndex = 0;
                const ufbx_material *material = nullptr;
                if (partIndex < node->materials.count)
                    material = node->materials.data[partIndex];
                else if (partIndex < mesh->materials.count)
                    material = mesh->materials.data[partIndex];
                if (material)
                {
                    const auto materialIterator = materialSlotMap.find(material);
                    if (materialIterator != materialSlotMap.end())
                        materialSlotIndex = materialIterator->second;
                }

                for (size_t faceListIndex = 0; faceListIndex < faceCount; ++faceListIndex)
                {
                    const uint32_t faceIndex = faceIndexList
                            ? faceIndexList[faceListIndex]
                            : static_cast<uint32_t>(faceListIndex);
                    if (faceIndex >= mesh->faces.count)
                        continue;

                    const ufbx_face face = mesh->faces.data[faceIndex];
                    if (face.num_indices < 3)
                        continue;

                    const size_t requiredCapacity = static_cast<size_t>(mesh->max_face_triangles) * 3u;
                    if (triangleIndicesScratch.size() < requiredCapacity)
                        triangleIndicesScratch.resize(requiredCapacity);

                    const uint32_t triangleCount = ufbx_triangulate_face(
                            triangleIndicesScratch.data(), triangleIndicesScratch.size(), mesh, face);
                    for (uint32_t triangleCorner = 0; triangleCorner < triangleCount * 3u; ++triangleCorner)
                    {
                        const uint32_t vertexIndex = triangleIndicesScratch[triangleCorner];
                        MeshVertex vertex;
                        const ufbx_vec3 localPosition =
                                ufbx_get_vertex_vec3(&mesh->vertex_position, vertexIndex);
                        vertex.Position = ToVec3(ufbx_transform_position(&geometryToWorld, localPosition));

                        if (mesh->vertex_normal.exists)
                        {
                            const ufbx_vec3 localNormal =
                                    ufbx_get_vertex_vec3(&mesh->vertex_normal, vertexIndex);
                            vertex.Normal = glm::normalize(
                                    ToVec3(ufbx_transform_direction(&normalMatrix, localNormal)));
                        }

                        if (mesh->vertex_uv.exists)
                        {
                            const ufbx_vec2 localUv = ufbx_get_vertex_vec2(&mesh->vertex_uv, vertexIndex);
                            vertex.TextureCoordinate = ToVec2(localUv);
                        }

                        if (mesh->vertex_tangent.exists)
                        {
                            const ufbx_vec3 localTangent =
                                    ufbx_get_vertex_vec3(&mesh->vertex_tangent, vertexIndex);
                            const glm::vec3 worldTangent = glm::normalize(
                                    ToVec3(ufbx_transform_direction(&normalMatrix, localTangent)));
                            float handedness = 1.0f;
                            if (mesh->vertex_bitangent.exists)
                            {
                                const ufbx_vec3 localBitangent =
                                        ufbx_get_vertex_vec3(&mesh->vertex_bitangent, vertexIndex);
                                const glm::vec3 worldBitangent = glm::normalize(
                                        ToVec3(ufbx_transform_direction(&normalMatrix, localBitangent)));
                                handedness = glm::dot(glm::cross(vertex.Normal, worldTangent), worldBitangent)
                                                             < 0.0f
                                                     ? -1.0f
                                                     : 1.0f;
                            }
                            vertex.Tangent = glm::vec4(worldTangent, handedness);
                        }
                        else
                        {
                            vertex.Tangent = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                        }

                        const uint32_t outputIndex = static_cast<uint32_t>(outMeshAsset.Vertices.size());
                        outMeshAsset.Vertices.push_back(vertex);
                        outMeshAsset.Indices.push_back(outputIndex);
                        ++addedIndexCount;
                    }
                }

                if (addedIndexCount == 0)
                    continue;

                MeshSubmesh submesh;
                submesh.IndexStart = indexStart;
                submesh.IndexCount = addedIndexCount;
                submesh.MaterialSlotIndex = materialSlotIndex;
                outMeshAsset.Submeshes.push_back(submesh);
            }
        }
    }

    bool LoadFbxMeshGeometry(const std::filesystem::path &filesystemPath, MeshAsset &outMeshAsset)
    {
        outMeshAsset.Vertices.clear();
        outMeshAsset.Indices.clear();
        outMeshAsset.Submeshes.clear();

        ufbx_load_opts loadOptions = {};
        loadOptions.generate_missing_normals = true;

        ufbx_error error = {};
        ufbx_scene *scene =
                ufbx_load_file(filesystemPath.string().c_str(), &loadOptions, &error);
        if (!scene)
        {
            char errorBuffer[512];
            ufbx_format_error(errorBuffer, sizeof(errorBuffer), &error);
            HIMII_CORE_ERROR("Failed to parse FBX '{0}': {1}", filesystemPath.string(), errorBuffer);
            return false;
        }

        std::unordered_map<const ufbx_material *, uint32_t> materialSlotMap;
        for (size_t materialIndex = 0; materialIndex < scene->materials.count; ++materialIndex)
            materialSlotMap[scene->materials.data[materialIndex]] = static_cast<uint32_t>(materialIndex);

        std::vector<uint32_t> triangleIndicesScratch;
        for (size_t nodeIndex = 0; nodeIndex < scene->nodes.count; ++nodeIndex)
            AppendMeshInstance(scene->nodes.data[nodeIndex], materialSlotMap, outMeshAsset,
                               triangleIndicesScratch);

        ufbx_free_scene(scene);

        if (outMeshAsset.Vertices.empty() || outMeshAsset.Indices.empty())
        {
            HIMII_CORE_ERROR("FBX contained no triangle geometry: {0}", filesystemPath.string());
            return false;
        }

        if (outMeshAsset.Submeshes.empty())
        {
            MeshSubmesh submesh;
            submesh.IndexStart = 0;
            submesh.IndexCount = static_cast<uint32_t>(outMeshAsset.Indices.size());
            submesh.MaterialSlotIndex = 0;
            outMeshAsset.Submeshes.push_back(submesh);
        }

        if (MeshNeedsGeneratedTangents(outMeshAsset))
            GenerateMeshTangents(outMeshAsset);

        return true;
    }
}
