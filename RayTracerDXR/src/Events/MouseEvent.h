#pragma once
#include "Event.h"

class MouseEvent : public Event
{
public:
	virtual EventCategory GetCategory() const override;
	virtual ~MouseEvent() = default;
};

class MouseButtonEvent : public MouseEvent
{
public:
	inline uint32_t GetMouseButtonCode() const { return static_cast<uint32_t>(Button); }
	virtual ~MouseButtonEvent() = default;
protected:
	MouseButtonEvent(MouseButtonCode button);

	MouseButtonCode Button;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
	MouseButtonPressedEvent(MouseButtonCode button);

	static EventType GetEventTypeStatic();
	virtual EventType GetEventType() const override;
	virtual constexpr const char* GetName() const override;
	virtual std::string GetEventInfo() const override;
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
	MouseButtonReleasedEvent(MouseButtonCode button);

	static EventType GetEventTypeStatic();
	virtual EventType GetEventType() const override;
	virtual constexpr const char* GetName() const override;
	virtual std::string GetEventInfo() const override;
};

class MouseMovedEvent : public MouseEvent
{
public:
	MouseMovedEvent(uint32_t x, uint32_t y);

	inline uint32_t GetXPos() const { return XPos; }
	inline uint32_t GetYPos() const { return YPos; }

	static EventType GetEventTypeStatic();
	virtual EventType GetEventType() const override;
	virtual constexpr const char* GetName() const override;
	virtual std::string GetEventInfo() const override;

private:
	uint32_t XPos, YPos;
};

class MouseScrolledEvent : public MouseEvent
{
public:
	MouseScrolledEvent(uint32_t xOffset, uint32_t yOffset, int delta);

	inline uint32_t GetXOffset() const { return XOffset; }
	inline uint32_t GetYOffset() const { return YOffset; }
	inline int GetDelta() const { return Delta; }

	static EventType GetEventTypeStatic();
	virtual EventType GetEventType() const override;
	virtual constexpr const char* GetName() const override;
	virtual std::string GetEventInfo() const override;

private:
	uint32_t XOffset, YOffset;
	int Delta;
};

class MouseEnterEvent : public MouseEvent
{
public:
	MouseEnterEvent() = default;

	static EventType GetEventTypeStatic();
	virtual EventType GetEventType() const override;
	virtual constexpr const char* GetName() const override;
	virtual std::string GetEventInfo() const override;
};

class MouseLeaveEvent : public MouseEvent
{
public:
	MouseLeaveEvent() = default;

	static EventType GetEventTypeStatic();
	virtual EventType GetEventType() const override;
	virtual constexpr const char* GetName() const override;
	virtual std::string GetEventInfo() const override;
};

class MouseRawInputEvent : public MouseEvent
{
public:
	MouseRawInputEvent(uint32_t x, uint32_t y);

	static EventType GetEventTypeStatic();
	virtual EventType GetEventType() const override;
	virtual constexpr const char* GetName() const override;
	virtual std::string GetEventInfo() const override;

	inline uint32_t GetX() const { return X; }
	inline uint32_t GetY() const { return Y; }

private:
	uint32_t X, Y;
};

struct MouseEventFactory : public EventFactoryBase
{
	virtual std::unique_ptr<Event> Make(Event* e) const override;
};