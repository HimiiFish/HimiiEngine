#include "World/Scene/Entity.h"
#include "World/Scene/Scene.h"

namespace Himii {

    Entity::Entity(entt::entity handle, Scene* scene)
        : m_EntityHandle(handle), m_Scene(scene) {}

} // namespace Himii
