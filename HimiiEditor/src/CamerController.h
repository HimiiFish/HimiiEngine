#pragma once
#include "Engine.h"

namespace Himii
{
    class CameraController : public ScriptableEntity 
    {
    protected:

        void OnCreate()
        {
            transform = &GetComponent<TransformComponent>().Position;
            std::cout << "hello world!" << std::endl;
        }

        void OnDestroy()
        {
        }

        void OnUpdate(Timestep ts)
        {
            if (Input::IsKeyPressed(Key::A))
                transform->x -= 0.05f;
            if (Input::IsKeyPressed(Key::D))
                transform->x += 0.05f;
            if (Input::IsKeyPressed(Key::W))
                transform->y += 0.05f;
            if (Input::IsKeyPressed(Key::S))
                transform->y -= 0.05f;
        }

    private:
        float m_CameraMoveSpeed = 1.0f;
        glm::vec3 *transform;
    };

} // namespace Himii
