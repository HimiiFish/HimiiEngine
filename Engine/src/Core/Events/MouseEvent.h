#pragma once
#include "Event.h"

namespace Core
{

    class MouseMovedEvent : public Event {
    public:
        MouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y)
        {
        }

        float GetX() const
        {
            return m_MouseX;
        }
        float GetY() const
        {
            return m_MouseY;
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)

    private:
        float m_MouseX, m_MouseY;
    };

    class MouseButtonEvent : public Event {
    public:
        int GetMouseButton() const
        {
            return m_Button;
        }

        EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton)

    protected:
        MouseButtonEvent(int button) : m_Button(button)
        {
        }
        int m_Button;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent {
    public:
        MouseButtonPressedEvent(int button) : MouseButtonEvent(button)
        {
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent {
    public:
        MouseButtonReleasedEvent(int button) : MouseButtonEvent(button)
        {
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };

} // namespace Core
