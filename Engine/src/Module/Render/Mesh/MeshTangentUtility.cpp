#include "Hepch.h"
#include "Module/Render/Mesh/MeshTangentUtility.h"
#include "Module/Render/Mesh/MeshAsset.h"

namespace Himii
{
    bool MeshNeedsGeneratedTangents(const MeshAsset &meshAsset)
    {
        if (meshAsset.Vertices.empty())
            return false;

        for (const MeshVertex &vertex : meshAsset.Vertices)
        {
            const float tangentLengthSquared =
                    vertex.Tangent.x * vertex.Tangent.x + vertex.Tangent.y * vertex.Tangent.y
                    + vertex.Tangent.z * vertex.Tangent.z;
            if (tangentLengthSquared < 1.0e-8f)
                return true;
        }
        return false;
    }

    void GenerateMeshTangents(MeshAsset &meshAsset)
    {
        if (meshAsset.Vertices.empty() || meshAsset.Indices.size() < 3)
            return;

        std::vector<glm::vec3> accumulatedTangents(meshAsset.Vertices.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> accumulatedBitangents(meshAsset.Vertices.size(), glm::vec3(0.0f));

        for (size_t indexOffset = 0; indexOffset + 2 < meshAsset.Indices.size(); indexOffset += 3)
        {
            const uint32_t index0 = meshAsset.Indices[indexOffset + 0];
            const uint32_t index1 = meshAsset.Indices[indexOffset + 1];
            const uint32_t index2 = meshAsset.Indices[indexOffset + 2];
            if (index0 >= meshAsset.Vertices.size() || index1 >= meshAsset.Vertices.size()
                || index2 >= meshAsset.Vertices.size())
                continue;

            const MeshVertex &vertex0 = meshAsset.Vertices[index0];
            const MeshVertex &vertex1 = meshAsset.Vertices[index1];
            const MeshVertex &vertex2 = meshAsset.Vertices[index2];

            const glm::vec3 edge1 = vertex1.Position - vertex0.Position;
            const glm::vec3 edge2 = vertex2.Position - vertex0.Position;
            const glm::vec2 deltaUv1 = vertex1.TextureCoordinate - vertex0.TextureCoordinate;
            const glm::vec2 deltaUv2 = vertex2.TextureCoordinate - vertex0.TextureCoordinate;

            const float determinant = deltaUv1.x * deltaUv2.y - deltaUv2.x * deltaUv1.y;
            if (std::abs(determinant) < 1.0e-8f)
                continue;

            const float inverseDeterminant = 1.0f / determinant;
            const glm::vec3 tangent =
                    (edge1 * deltaUv2.y - edge2 * deltaUv1.y) * inverseDeterminant;
            const glm::vec3 bitangent =
                    (edge2 * deltaUv1.x - edge1 * deltaUv2.x) * inverseDeterminant;

            accumulatedTangents[index0] += tangent;
            accumulatedTangents[index1] += tangent;
            accumulatedTangents[index2] += tangent;
            accumulatedBitangents[index0] += bitangent;
            accumulatedBitangents[index1] += bitangent;
            accumulatedBitangents[index2] += bitangent;
        }

        for (size_t vertexIndex = 0; vertexIndex < meshAsset.Vertices.size(); ++vertexIndex)
        {
            MeshVertex &vertex = meshAsset.Vertices[vertexIndex];
            const glm::vec3 normal = glm::normalize(vertex.Normal);
            glm::vec3 tangent = accumulatedTangents[vertexIndex];
            if (glm::dot(tangent, tangent) < 1.0e-8f)
            {
                // 退化 UV：任选与法线正交的轴。
                const glm::vec3 helper =
                        std::abs(normal.y) < 0.999f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
                tangent = glm::normalize(glm::cross(helper, normal));
                vertex.Tangent = glm::vec4(tangent, 1.0f);
                continue;
            }

            tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
            const glm::vec3 bitangent = accumulatedBitangents[vertexIndex];
            const float handedness =
                    glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f ? -1.0f : 1.0f;
            vertex.Tangent = glm::vec4(tangent, handedness);
        }
    }
}
