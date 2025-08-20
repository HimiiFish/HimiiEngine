#pragma once
#include "Himii/Events/Event.h"
#include "Himii/Core/Timestep.h"
#include <string>

namespace Himii
{
    class Layer {
    public:
        explicit Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(Timestep ts) {}
        virtual void OnImGuiRender() {}
        virtual void OnEvent(Event& event) {}

        const std::string& GetName() const noexcept { return m_DebugName; }

    protected:
        std::string m_DebugName;
    };
} // namespace Himii
