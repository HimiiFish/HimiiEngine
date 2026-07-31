#pragma once

#include "EngineCore/Core/Timestep.h"
#include "World/Scene/Scene.h"
#include "World/Scene/Entity.h"

#include "box2d/box2d.h"

namespace Himii
{
    /// Box2D 2D 物理世界实现；由 Physics2DModule 持有，绑定到一个 Scene。
    class Physics2DWorld
    {
    public:
        explicit Physics2DWorld(Scene *scene);
        ~Physics2DWorld();

        Physics2DWorld(const Physics2DWorld &) = delete;
        Physics2DWorld &operator=(const Physics2DWorld &) = delete;

        void Start();
        void Stop();
        void Step(Timestep timestep);

        Scene::RaycastHit2D Raycast2D(glm::vec2 start, glm::vec2 end);
        void SyncEntityTransform(Entity entity);

        bool IsRunning() const { return b2World_IsValid(m_Box2DWorld); }

    private:
        Entity GetEntityFromShape(b2ShapeId shapeId);
        void ProcessContacts();

        Scene *m_Scene = nullptr;
        b2WorldId m_Box2DWorld{};
    };
}
