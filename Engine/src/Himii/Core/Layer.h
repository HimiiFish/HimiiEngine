#pragma once
#include "Himii/Events/Event.h"
#include "Himii/Core/Timestep.h"

namespace Himii
{
    class Layer {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer()=default;

        virtual void OnAttach()
        {
        }

        virtual void OnDetach()
        {
        }
        
        virtual void OnUpdate(Timestep ts)
        {
        }

        virtual void OnImGuiRender()
        {
        }

        virtual void OnEvent(Event& event)
        {
        }

        const std::string &GetName() const
        {
            return m_DebugName;
        }

    private:
        std::string m_DebugName;
    };
} // namespace Core
