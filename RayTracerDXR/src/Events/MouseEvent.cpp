#include "MouseEvent.h"
#include <sstream>

MouseButtonEvent::MouseButtonEvent(MouseButtonCode button)
	:Button(button)
{
}

MouseButtonPressedEvent::MouseButtonPressedEvent(MouseButtonCode button)
	:MouseButtonEvent(button)
{
}

EventType MouseButtonPressedEvent::GetEventTypeStatic()
{
	return EventType::MouseButtonPressed;
}

EventType MouseButtonPressedEvent::GetEventType() const
{
	return EventType::MouseButtonPressed;
}

constexpr const char* MouseButtonPressedEvent::GetName() const
{
	return "MouseButtonPressedEvent";
}

std::string MouseButtonPressedEvent::GetEventInfo() const
{
	std::stringstream ss;
	ss << GetName() << "| MouseButtonCode: " << static_cast<int>(Button);
	return ss.str();
}

MouseButtonReleasedEvent::MouseButtonReleasedEvent(MouseButtonCode button)
	:MouseButtonEvent(button)
{
}

EventType MouseButtonReleasedEvent::GetEventTypeStatic()
{
	return EventType::MouseButtonReleased;
}

EventType MouseButtonReleasedEvent::GetEventType() const
{
	return EventType::MouseButtonReleased;
}

constexpr const char* MouseButtonReleasedEvent::GetName() const
{
	return "MouseButtonReleasedEvent";
}

std::string MouseButtonReleasedEvent::GetEventInfo() const
{
	std::stringstream ss;
	ss << GetName() << "| MouseButtonCode: " << static_cast<int>(Button);
	return ss.str();
}

MouseMovedEvent::MouseMovedEvent(uint32_t x, uint32_t y)
	:XPos(x), YPos(y)
{
}

EventType MouseMovedEvent::GetEventTypeStatic()
{
	return EventType::MouseMoved;
}

EventType MouseMovedEvent::GetEventType() const
{
	return EventType::MouseMoved;
}

constexpr const char* MouseMovedEvent::GetName() const
{
	return "MouseMovedEvent";
}

std::string MouseMovedEvent::GetEventInfo() const
{
	return MouseMovedEvent::GetName();
}

MouseScrolledEvent::MouseScrolledEvent(uint32_t xOffset, uint32_t yOffset, int delta)
	:XOffset(xOffset), YOffset(yOffset), Delta(delta)
{
}

EventType MouseScrolledEvent::GetEventTypeStatic()
{
	return EventType::MouseScrolled;
}

EventType MouseScrolledEvent::GetEventType() const
{
	return EventType::MouseScrolled;
}

constexpr const char* MouseScrolledEvent::GetName() const
{
	return "MouseScrolledEvent";
}

std::string MouseScrolledEvent::GetEventInfo() const
{
	return MouseScrolledEvent::GetName();
}

EventType MouseEnterEvent::GetEventTypeStatic()
{
	return EventType::MouseEnter;
}

EventType MouseEnterEvent::GetEventType() const
{
	return EventType::MouseEnter;
}

constexpr const char* MouseEnterEvent::GetName() const
{
	return "MouseEnterEvent";
}

std::string MouseEnterEvent::GetEventInfo() const
{
	return MouseEnterEvent::GetName();
}

EventType MouseLeaveEvent::GetEventTypeStatic()
{
	return EventType::MouseLeave;
}

EventType MouseLeaveEvent::GetEventType() const
{
	return EventType::MouseLeave;
}

constexpr const char* MouseLeaveEvent::GetName() const
{
	return "MouseLeaveEvent";
}

std::string MouseLeaveEvent::GetEventInfo() const
{
	return MouseLeaveEvent::GetName();
}

MouseRawInputEvent::MouseRawInputEvent(uint32_t x, uint32_t y)
	: X(x), Y(y)
{}

EventType MouseRawInputEvent::GetEventTypeStatic()
{
	return EventType::MouseRaw;
}

EventType MouseRawInputEvent::GetEventType() const
{
	return EventType::MouseRaw;
}

constexpr const char* MouseRawInputEvent::GetName() const
{
	return "MouseRawInputEvent";
}

std::string MouseRawInputEvent::GetEventInfo() const
{
	return MouseRawInputEvent::GetName();
}

std::unique_ptr<Event> MouseEventFactory::Make(Event* e) const
{
	const auto& typeID = typeid(*e);
	if (typeID == typeid(MouseButtonPressedEvent))
		return std::unique_ptr<MouseButtonPressedEvent>(dynamic_cast<MouseButtonPressedEvent*>(e));
	else if (typeID == typeid(MouseButtonReleasedEvent))
		return std::unique_ptr<MouseButtonReleasedEvent>(dynamic_cast<MouseButtonReleasedEvent*>(e));
	else if (typeID == typeid(MouseMovedEvent))
		return std::unique_ptr<MouseMovedEvent>(dynamic_cast<MouseMovedEvent*>(e));
	else if (typeID == typeid(MouseScrolledEvent))
		return std::unique_ptr<MouseScrolledEvent>(dynamic_cast<MouseScrolledEvent*>(e));
	else if (typeID == typeid(MouseEnterEvent))
		return std::unique_ptr<MouseEnterEvent>(dynamic_cast<MouseEnterEvent*>(e));
	else if (typeID == typeid(MouseLeaveEvent))
		return std::unique_ptr<MouseLeaveEvent>(dynamic_cast<MouseLeaveEvent*>(e));
	else if (typeID == typeid(MouseRawInputEvent))
		return std::unique_ptr<MouseRawInputEvent>(dynamic_cast<MouseRawInputEvent*>(e));
	else
		return nullptr;

}

EventCategory MouseEvent::GetCategory() const
{
	return EventCategory::MouseEvents;
}
