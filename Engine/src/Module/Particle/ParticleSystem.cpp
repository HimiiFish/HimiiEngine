#include "Hepch.h"
#include "Module/Particle/ParticleSystem.h"

#include <random>

namespace Himii
{
    namespace
    {
        // 返回 [0, 1) 之间的随机浮点数
        float RandomFloat()
        {
            static thread_local std::mt19937 generator{ std::random_device{}() };
            static thread_local std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
            return distribution(generator);
        }
    }

    ParticleSystem::ParticleSystem(std::uint32_t maxParticles)
        : _particlePool(maxParticles)
        , _poolIndex(maxParticles ? maxParticles - 1u : 0u)
    {
    }

    void ParticleSystem::Emit(const ParticleProps& props)
    {
        if (_particlePool.empty())
            return;

        // 循环复用粒子池中的槽位
        Particle& particle = _particlePool[_poolIndex];

        particle.active = true;
        particle.position = props.position;

        // 每个分量独立随机扰动，形成锥形或扇形效果
        glm::vec3 randomDir{
            (RandomFloat() - 0.5f) * 2.0f,
            (RandomFloat() - 0.5f) * 2.0f,
            (RandomFloat() - 0.5f) * 2.0f
        };
        particle.velocity = props.velocity + randomDir * props.velocityVariation;

        particle.rotation = 0.0f;

        particle.lifetime = props.lifetime;
        particle.remainingLife = props.lifetime;

        particle.colorBegin = props.colorBegin;
        particle.colorEnd = props.colorEnd;

        particle.sizeBegin = props.sizeBegin;
        particle.sizeEnd = props.sizeEnd;

        particle.shape = props.shape;
        particle.textureHandle = props.textureHandle;

        // 更新下一个可用槽位索引（向前循环）
        if (_poolIndex == 0)
            _poolIndex = static_cast<std::uint32_t>(_particlePool.size() - 1);
        else
            --_poolIndex;
    }

    void ParticleSystem::OnUpdate(float deltaTime)
    {
        for (auto& particle : _particlePool)
        {
            if (!particle.active)
                continue;

            particle.remainingLife -= deltaTime;
            if (particle.remainingLife <= 0.0f)
            {
                particle.active = false;
                continue;
            }

            // 简单的欧拉积分更新：后续可扩展加重力、阻尼等
            particle.position += particle.velocity * deltaTime;
            // 如果需要，可以加入旋转速度，这里先保持为 0
        }
    }
}

