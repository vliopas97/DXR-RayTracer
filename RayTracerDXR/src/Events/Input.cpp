#include "Input.h"

#include <typeinfo>
#include <unordered_map>

class EventFactory
{
	std::unordered_map<EventCategory, std::unique_ptr<EventFactoryBase>> EventFactories;
public:
	EventFactory()
	{
		EventFactories[EventCategory::MouseEvents] = std::make_unique<MouseEventFactory>();
		EventFactories[EventCategory::KeyEvents] = std::make_unique<KeyEventFactory>();
	}

	std::unique_ptr<Event> Make(Event* e)
	{
		return EventFactories[e->GetCategory()]->Make(e);
	}
};

void InputManager::FlushKeyEventBuffer() noexcept
{
	KeyEventBuffer = std::list<std::unique_ptr<Event>>();
}

std::optional<std::unique_ptr<Event>> InputManager::FetchKeyEvent() noexcept
{
	if (KeyEventBuffer.size() > 0)
	{
		auto eventPtr = KeyEventBuffer.front().release();
		KeyEventBuffer.pop_front();
		EventFactory eventGenerator;
		return std::optional<std::unique_ptr<Event>>(eventGenerator.Make(eventPtr));
	}
	else
		return std::nullopt;
}

std::optional<std::unique_ptr<Event>> InputManager::FetchMouseEvent() noexcept
{
	if (MouseEventBuffer.size() > 0)
	{
		auto eventPtr = MouseEventBuffer.front().release();
		MouseEventBuffer.pop();
		EventFactory eventGenerator;
		return std::optional<std::unique_ptr<Event>>(eventGenerator.Make(eventPtr));
	}
	else
		return std::nullopt;
}

std::optional<InputManager::RawInputCoords> InputManager::FetchRawInputCoords() noexcept
{
	if (RawInputBuffer.empty())
		return std::nullopt;

	auto rawInputCoords = RawInputBuffer.front();
	RawInputBuffer.pop();
	return rawInputCoords;
}

void InputManager::FlushRawInputBuffer() noexcept
{
	RawInputBuffer = std::queue<RawInputCoords>();
}

void InputManager::FlushCharBuffer() noexcept
{
	CharBuffer = std::queue<uint8>();
}

uint8 InputManager::FetchKeyTyped() noexcept
{
	if (CharBuffer.size() > 0)
	{
		uint8 charCode = CharBuffer.front();
		CharBuffer.pop();
		return charCode;
	}
	else
		return 0;
}

void InputManager::ResetKeyStates() noexcept
{
	KeyStates.reset();
}

void InputManager::FlushAll() noexcept
{
	FlushKeyEventBuffer();
	FlushCharBuffer();
	FlushRawInputBuffer();
}

void InputManager::SetAutoRepeat(bool autoRepeat) noexcept
{
	RepeatEnabled = autoRepeat;
}

bool InputManager::IsKeyPressed(uint8 keycode) noexcept
{
	for (auto it = KeyEventBuffer.begin(); it != KeyEventBuffer.end(); it++)
	{
		auto ptr = dynamic_cast<KeyPressedEvent*>(it->get());
		if (ptr && ptr->GetKeycode() == keycode)
		{
			KeyEventBuffer.erase(it);
			return true;
		}
	}
	return false;
}

bool InputManager::IsKeyBufferEmpty() const noexcept
{
	return KeyEventBuffer.empty();
}

bool InputManager::IsMouseBufferEmpty() const noexcept
{
	return MouseEventBuffer.empty();
}

bool InputManager::IsAutoRepeatEnabled() const noexcept
{
	return RepeatEnabled;
}

bool InputManager::IsMouseInWindow() const noexcept
{
	return MouseInWindow;
}

bool InputManager::IsCharBufferEmpty() const noexcept
{
	return CharBuffer.empty();
}

void InputManager::SetRawInput(bool flag)
{
	RawInputEnabled = flag;
}

bool InputManager::IsRawInputEnabled() const
{
	return RawInputEnabled;
}

void InputManager::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);

	dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(OnKeyPressed));
	dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(OnKeyReleased));
	dispatcher.Dispatch<KeyTypedEvent>(BIND_EVENT_FN(OnKeyTyped));

	dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(OnMouseButtonPressed));
	dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(OnMouseButtonReleased));
	dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(OnMouseMoved));
	dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(OnMouseScrolled));
	dispatcher.Dispatch<MouseEnterEvent>(BIND_EVENT_FN(OnMouseEnter));
	dispatcher.Dispatch<MouseLeaveEvent>(BIND_EVENT_FN(OnMouseLeave));
	dispatcher.Dispatch<MouseRawInputEvent>(BIND_EVENT_FN(OnMouseRawInput));
}

bool InputManager::OnKeyPressed(KeyPressedEvent& event) noexcept
{
	if (!IsAutoRepeatEnabled() && event.IsRepeated())
		return true;

	KeyStates[event.GetKeycode()] = true;
	KeyEventBuffer.emplace_back(std::make_unique<KeyPressedEvent>(event.GetKeycode(), event.IsRepeated()));
	PreventBufferOverflow(KeyEventBuffer);
	return true;
}

bool InputManager::OnKeyReleased(KeyReleasedEvent& event) noexcept
{
	KeyStates[event.GetKeycode()] = false;
	KeyEventBuffer.emplace_back(std::make_unique<KeyReleasedEvent>(event.GetKeycode()));
	PreventBufferOverflow(KeyEventBuffer);
	return true;
}

bool InputManager::OnKeyTyped(KeyTypedEvent& event) noexcept
{
	CharBuffer.push(event.GetKeycode());
	PreventBufferOverflow(KeyEventBuffer);
	return true;
}

bool InputManager::OnMouseButtonPressed(MouseButtonPressedEvent& event) noexcept
{
	MouseStates[event.GetMouseButtonCode()] = true;
	MouseEventBuffer.emplace(std::make_unique<MouseButtonPressedEvent>(static_cast<MouseButtonCode>(event.GetMouseButtonCode())));
	PreventBufferOverflow(MouseEventBuffer);
	return true;
}

bool InputManager::OnMouseButtonReleased(MouseButtonReleasedEvent& event) noexcept
{
	MouseStates[event.GetMouseButtonCode()] = false;
	MouseEventBuffer.emplace(std::make_unique<MouseButtonReleasedEvent>(static_cast<MouseButtonCode>(event.GetMouseButtonCode())));
	PreventBufferOverflow(MouseEventBuffer);
	return true;
}

bool InputManager::OnMouseMoved(MouseMovedEvent& event) noexcept
{
	MouseEventBuffer.emplace(std::make_unique<MouseMovedEvent>(event.GetXPos(), event.GetYPos()));
	PreventBufferOverflow(MouseEventBuffer);
	return true;
}

bool InputManager::OnMouseScrolled(MouseScrolledEvent& event) noexcept
{
	DeltaCarry += event.GetDelta();

	while (std::abs(DeltaCarry) >= WHEEL_DELTA)
	{
		if (DeltaCarry > 0)
			DeltaCarry -= WHEEL_DELTA;
		else
			DeltaCarry += WHEEL_DELTA;
	}
	MouseEventBuffer.emplace(std::make_unique<MouseScrolledEvent>(event.GetXOffset(), event.GetYOffset(), event.GetDelta()));
	PreventBufferOverflow(MouseEventBuffer);

	return true;
}

bool InputManager::OnMouseEnter(MouseEnterEvent& event) noexcept
{
	MouseInWindow = true;
	MouseEventBuffer.emplace(std::make_unique<MouseEnterEvent>());
	PreventBufferOverflow(MouseEventBuffer);
	return true;
}

bool InputManager::OnMouseLeave(MouseLeaveEvent& event) noexcept
{
	MouseInWindow = false;
	MouseEventBuffer.emplace(std::make_unique<MouseLeaveEvent>());
	PreventBufferOverflow(MouseEventBuffer);
	return true;
}

bool InputManager::OnMouseRawInput(MouseRawInputEvent& event) noexcept
{
	if (!RawInputEnabled)
		return false;

	RawInputBuffer.push({ event.GetX(), event.GetY() });
	PreventBufferOverflow(RawInputBuffer);
	return true;
}

template void InputManager::PreventBufferOverflow<KeyEvent>(std::queue<KeyEvent>&) noexcept;
template void InputManager::PreventBufferOverflow<uint8>(std::queue<uint8>&) noexcept;

template<typename T>
void InputManager::PreventBufferOverflow(std::queue<T>& buffer) noexcept
{
	while (buffer.size() > BufferSize)
		buffer.pop();
}

template void InputManager::PreventBufferOverflow<std::unique_ptr<Event>>(std::list<std::unique_ptr<Event>>&) noexcept;

template<typename T>
void InputManager::PreventBufferOverflow(std::list<T>& buffer) noexcept
{
	while (buffer.size() > BufferSize)
		buffer.pop_front();
}
