#pragma once

#include <vector>
#include <cstdint>

#include <glm/glm.hpp>

namespace Himii
{
    enum class ParticleShape : uint8_t
    {
        Quad = 0,
        Circle = 1
    };

    struct ParticleProps
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        glm::vec3 velocityVariation{ 0.0f };

        float lifetime = 1.0f;

        glm::vec4 colorBegin{ 1.0f };
        glm::vec4 colorEnd{ 0.0f };

        float sizeBegin = 1.0f;
        float sizeEnd = 0.0f;

        ParticleShape shape = ParticleShape::Quad;
        uint64_t textureHandle = 0;  // 0 = 无贴图，仅用颜色
    };

    class ParticleSystem
    {
    public:
        explicit ParticleSystem(std::uint32_t maxParticles = 10000);

        void Emit(const ParticleProps& props);

        // 仅负责更新粒子状态（模拟），与渲染解耦
        void OnUpdate(float deltaTime);

        struct ParticleView
        {
            glm::vec3 position;
            float rotation;
            float remainingLife;
            float lifetime;
            glm::vec4 colorBegin;
            glm::vec4 colorEnd;
            float sizeBegin;
            float sizeEnd;
            ParticleShape shape;
            uint64_t textureHandle;
        };

        template<typename Func>
        void ForEachAlive(Func&& func) const
        {
            for (const auto& particle : _particlePool)
            {
                if (!particle.active)
                    continue;

                ParticleView view{
                    particle.position,
                    particle.rotation,
                    particle.remainingLife,
                    particle.lifetime,
                    particle.colorBegin,
                    particle.colorEnd,
                    particle.sizeBegin,
                    particle.sizeEnd,
                    particle.shape,
                    particle.textureHandle
                };

                func(view);
            }
        }

    private:
        struct Particle
        {
            glm::vec3 position{ 0.0f };
            glm::vec3 velocity{ 0.0f };
            float rotation = 0.0f;

            float lifetime = 1.0f;
            float remainingLife = 0.0f;

            glm::vec4 colorBegin{ 1.0f };
            glm::vec4 colorEnd{ 0.0f };

            float sizeBegin = 1.0f;
            float sizeEnd = 0.0f;

            ParticleShape shape = ParticleShape::Quad;
            uint64_t textureHandle = 0;

            bool active = false;
        };

        std::vector<Particle> _particlePool;
        std::uint32_t _poolIndex = 0;
    };
}

