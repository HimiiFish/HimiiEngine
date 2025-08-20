#pragma once
#include "Layer.h"
#include <vector>
#include <algorithm>

namespace Himii
{
    class LayerStack {
    public:
        LayerStack() = default;
        ~LayerStack();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

        std::vector<Layer*>::iterator begin() noexcept { return m_Layers.begin(); }
        std::vector<Layer*>::iterator end() noexcept { return m_Layers.end(); }
        std::vector<Layer*>::reverse_iterator rbegin() noexcept { return m_Layers.rbegin(); }
        std::vector<Layer*>::reverse_iterator rend() noexcept { return m_Layers.rend(); }
        std::vector<Layer*>::const_iterator begin() const noexcept { return m_Layers.begin(); }
        std::vector<Layer*>::const_iterator end() const noexcept { return m_Layers.end(); }
        std::vector<Layer*>::const_reverse_iterator rbegin() const noexcept { return m_Layers.rbegin(); }
        std::vector<Layer*>::const_reverse_iterator rend() const noexcept { return m_Layers.rend(); }

    private:
        std::vector<Layer*> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
} // namespace Himii
