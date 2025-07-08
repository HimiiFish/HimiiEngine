#pragma once
#include "Himii/Events/Event.h"

namespace Engine
{
    class Layer {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void OnAttach()
        {
        }

        virtual void OnDetach()
        {
        }
        
        virtual void OnUpdate()
        {
        }
        virtual void OnEvent(void *event)
        {
        }

        inline const std::string &GetName() const
        {
            return m_DebugName;
        }

    private:
        std::string m_DebugName;
    };
} // namespace Core
