#pragma once
#include "Himii/Core/Timestep.h"
#include "Himii/Scene/Entity.h"

namespace Himii {

class ScriptableEntity {
public:
    virtual ~ScriptableEntity() = default;

    template<typename T>
    T& GetComponent() { return m_Entity.GetComponent<T>(); }

protected:
    virtual void OnCreate() {}
    virtual void OnDestroy() {}
    virtual void OnUpdate(Timestep) {}

private:
    Entity m_Entity{};
    friend class Scene;
};

} // namespace Himii
