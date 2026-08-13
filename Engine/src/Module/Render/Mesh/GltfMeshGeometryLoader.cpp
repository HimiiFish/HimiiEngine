#include "Hepch.h"
#include "Module/Render/Mesh/GltfMeshGeometryLoader.h"
#include "Module/Render/Mesh/MeshTangentUtility.h"
#include "EngineCore/Core/Log.h"

#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <unordered_map>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace Himii
{
    namespace
    {
        bool ReadAccessorFloats(const cgltf_accessor *accessor, std::vector<float> &outValues, uint32_t componentCount)
        {
            if (!accessor)
                return false;

            outValues.resize(static_cast<size_t>(accessor->count) * componentCount);
            for (cgltf_size elementIndex = 0; elementIndex < accessor->count; ++elementIndex)
            {
                float temporary[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                if (!cgltf_accessor_read_float(accessor, elementIndex, temporary, componentCount))
                    return false;
                for (uint32_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
                    outValues[static_cast<size_t>(elementIndex) * componentCount + componentIndex] =
                            temporary[componentIndex];
            }
            return true;
        }

        bool ReadAccessorIndices(const cgltf_accessor *accessor, std::vector<uint32_t> &outIndices)
        {
            if (!accessor)
                return false;

            outIndices.resize(static_cast<size_t>(accessor->count));
            for (cgltf_size elementIndex = 0; elementIndex < accessor->count; ++elementIndex)
            {
                outIndices[elementIndex] =
                        static_cast<uint32_t>(cgltf_accessor_read_index(accessor, elementIndex));
            }
            return true;
        }

        const cgltf_attribute *FindAttribute(const cgltf_primitive *primitive, cgltf_attribute_type type)
        {
            for (cgltf_size attributeIndex = 0; attributeIndex < primitive->attributes_count; ++attributeIndex)
            {
                if (primitive->attributes[attributeIndex].type == type)
                    return &primitive->attributes[attributeIndex];
            }
            return nullptr;
        }

        void AppendPrimitive(const cgltf_primitive *primitive, const glm::mat4 &worldTransform,
                             const std::unordered_map<const cgltf_material *, uint32_t> &materialSlotMap,
                             MeshAsset &outMeshAsset)
        {
            if (!primitive || primitive->type != cgltf_primitive_type_triangles)
                return;

            const cgltf_attribute *positionAttribute = FindAttribute(primitive, cgltf_attribute_type_position);
            if (!positionAttribute || !positionAttribute->data)
                return;

            std::vector<float> positions;
            if (!ReadAccessorFloats(positionAttribute->data, positions, 3))
                return;

            std::vector<float> normals;
            const cgltf_attribute *normalAttribute = FindAttribute(primitive, cgltf_attribute_type_normal);
            if (normalAttribute && normalAttribute->data)
                ReadAccessorFloats(normalAttribute->data, normals, 3);

            std::vector<float> textureCoordinates;
            const cgltf_attribute *textureCoordinateAttribute =
                    FindAttribute(primitive, cgltf_attribute_type_texcoord);
            if (textureCoordinateAttribute && textureCoordinateAttribute->data)
                ReadAccessorFloats(textureCoordinateAttribute->data, textureCoordinates, 2);

            std::vector<float> tangents;
            const cgltf_attribute *tangentAttribute = FindAttribute(primitive, cgltf_attribute_type_tangent);
            if (tangentAttribute && tangentAttribute->data)
                ReadAccessorFloats(tangentAttribute->data, tangents, 4);

            std::vector<uint32_t> localIndices;
            if (primitive->indices)
            {
                if (!ReadAccessorIndices(primitive->indices, localIndices))
                    return;
            }
            else
            {
                const uint32_t vertexCount = static_cast<uint32_t>(positionAttribute->data->count);
                localIndices.resize(vertexCount);
                for (uint32_t index = 0; index < vertexCount; ++index)
                    localIndices[index] = index;
            }

            const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldTransform)));
            const uint32_t vertexBase = static_cast<uint32_t>(outMeshAsset.Vertices.size());
            const uint32_t vertexCount = static_cast<uint32_t>(positionAttribute->data->count);

            outMeshAsset.Vertices.reserve(outMeshAsset.Vertices.size() + vertexCount);
            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                MeshVertex vertex;
                const glm::vec3 localPosition(positions[vertexIndex * 3 + 0], positions[vertexIndex * 3 + 1],
                                              positions[vertexIndex * 3 + 2]);
                vertex.Position = glm::vec3(worldTransform * glm::vec4(localPosition, 1.0f));

                if (normals.size() >= static_cast<size_t>(vertexCount) * 3)
                {
                    const glm::vec3 localNormal(normals[vertexIndex * 3 + 0], normals[vertexIndex * 3 + 1],
                                                normals[vertexIndex * 3 + 2]);
                    vertex.Normal = glm::normalize(normalMatrix * localNormal);
                }
                else
                {
                    vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                if (textureCoordinates.size() >= static_cast<size_t>(vertexCount) * 2)
                {
                    vertex.TextureCoordinate = {textureCoordinates[vertexIndex * 2 + 0],
                                                textureCoordinates[vertexIndex * 2 + 1]};
                }

                if (tangents.size() >= static_cast<size_t>(vertexCount) * 4)
                {
                    const glm::vec3 localTangent(tangents[vertexIndex * 4 + 0], tangents[vertexIndex * 4 + 1],
                                                 tangents[vertexIndex * 4 + 2]);
                    const glm::vec3 worldTangent = glm::normalize(normalMatrix * localTangent);
                    vertex.Tangent = glm::vec4(worldTangent, tangents[vertexIndex * 4 + 3]);
                }
                else
                {
                    vertex.Tangent = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                }

                outMeshAsset.Vertices.push_back(vertex);
            }

            MeshSubmesh submesh;
            submesh.IndexStart = static_cast<uint32_t>(outMeshAsset.Indices.size());
            submesh.IndexCount = static_cast<uint32_t>(localIndices.size());
            submesh.MaterialSlotIndex = 0;
            if (primitive->material)
            {
                auto materialIterator = materialSlotMap.find(primitive->material);
                if (materialIterator != materialSlotMap.end())
                    submesh.MaterialSlotIndex = materialIterator->second;
            }

            outMeshAsset.Indices.reserve(outMeshAsset.Indices.size() + localIndices.size());
            for (uint32_t localIndex : localIndices)
                outMeshAsset.Indices.push_back(vertexBase + localIndex);
            outMeshAsset.Submeshes.push_back(submesh);
        }

        void ProcessNode(const cgltf_node *node, const std::unordered_map<const cgltf_material *, uint32_t> &materialSlotMap,
                         MeshAsset &outMeshAsset)
        {
            if (!node)
                return;

            cgltf_float worldMatrix[16];
            cgltf_node_transform_world(node, worldMatrix);
            const glm::mat4 worldTransform = glm::make_mat4(worldMatrix);

            if (node->mesh)
            {
                for (cgltf_size primitiveIndex = 0; primitiveIndex < node->mesh->primitives_count; ++primitiveIndex)
                    AppendPrimitive(&node->mesh->primitives[primitiveIndex], worldTransform, materialSlotMap,
                                    outMeshAsset);
            }

            for (cgltf_size childIndex = 0; childIndex < node->children_count; ++childIndex)
                ProcessNode(node->children[childIndex], materialSlotMap, outMeshAsset);
        }
    }

    bool LoadGltfMeshGeometry(const std::filesystem::path &filesystemPath, MeshAsset &outMeshAsset)
    {
        outMeshAsset.Vertices.clear();
        outMeshAsset.Indices.clear();
        outMeshAsset.Submeshes.clear();

        cgltf_options options = {};
        cgltf_data *data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, filesystemPath.string().c_str(), &data);
        if (result != cgltf_result_success || !data)
        {
            HIMII_CORE_ERROR("Failed to parse glTF/GLB: {0}", filesystemPath.string());
            return false;
        }

        result = cgltf_load_buffers(&options, data, filesystemPath.string().c_str());
        if (result != cgltf_result_success)
        {
            HIMII_CORE_ERROR("Failed to load glTF buffers: {0}", filesystemPath.string());
            cgltf_free(data);
            return false;
        }

        std::unordered_map<const cgltf_material *, uint32_t> materialSlotMap;
        for (cgltf_size materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
            materialSlotMap[&data->materials[materialIndex]] = static_cast<uint32_t>(materialIndex);

        if (data->scenes_count > 0 && data->scene)
        {
            for (cgltf_size nodeIndex = 0; nodeIndex < data->scene->nodes_count; ++nodeIndex)
                ProcessNode(data->scene->nodes[nodeIndex], materialSlotMap, outMeshAsset);
        }
        else if (data->nodes_count > 0)
        {
            for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
            {
                if (data->nodes[nodeIndex].parent == nullptr)
                    ProcessNode(&data->nodes[nodeIndex], materialSlotMap, outMeshAsset);
            }
        }
        else
        {
            // 无场景节点时，直接合并所有 mesh 原始体（单位变换）。
            const glm::mat4 identity(1.0f);
            for (cgltf_size meshIndex = 0; meshIndex < data->meshes_count; ++meshIndex)
            {
                for (cgltf_size primitiveIndex = 0; primitiveIndex < data->meshes[meshIndex].primitives_count;
                     ++primitiveIndex)
                {
                    AppendPrimitive(&data->meshes[meshIndex].primitives[primitiveIndex], identity, materialSlotMap,
                                    outMeshAsset);
                }
            }
        }

        cgltf_free(data);

        if (outMeshAsset.Vertices.empty() || outMeshAsset.Indices.empty())
        {
            HIMII_CORE_ERROR("glTF/GLB contained no triangle geometry: {0}", filesystemPath.string());
            return false;
        }

        // 若模型没有 material，保证至少一个默认槽。
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
