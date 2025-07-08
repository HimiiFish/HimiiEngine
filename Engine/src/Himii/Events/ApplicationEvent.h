#pragma once
#include "Event.h"

namespace Engine
{

    class WindowCloseEvent : public Event {
    public:
        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class WindowResizeEvent : public Event {
    public:
        WindowResizeEvent(int width, int height) : m_Width(width), m_Height(height)
        {
        }

        int GetWidth() const
        {
            return m_Width;
        }
        int GetHeight() const
        {
            return m_Height;
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        int m_Width, m_Height;
    };

} // namespace Core
