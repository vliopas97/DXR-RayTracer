#pragma once
#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Event.h"
#include "KeyEvent.h"
#include "MouseEvent.h"

#include <queue>
#include <bitset>

class InputManager
{
	using RawInputCoords = std::pair<int, int>;
public:
	InputManager() = default;
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	void FlushKeyEventBuffer() noexcept;
	std::optional<std::unique_ptr<Event>> FetchKeyEvent() noexcept;
	std::optional<std::unique_ptr<Event>> FetchMouseEvent() noexcept;
	std::optional<RawInputCoords> FetchRawInputCoords() noexcept;

	void FlushRawInputBuffer() noexcept;

	void FlushCharBuffer() noexcept;
	uint8 FetchKeyTyped() noexcept;

	void ResetKeyStates() noexcept;
	void FlushAll() noexcept;

	void SetAutoRepeat(bool autoRepeat) noexcept;

	bool IsKeyPressed(uint8 keycode) noexcept;
	bool IsKeyBufferEmpty() const noexcept;
	bool IsMouseBufferEmpty() const noexcept;
	bool IsAutoRepeatEnabled() const noexcept;
	bool IsMouseInWindow() const noexcept;
	bool IsCharBufferEmpty() const noexcept;

	void SetRawInput(bool flag);
	bool IsRawInputEnabled() const;

	void OnEvent(Event& event);
private:
	bool OnKeyPressed(KeyPressedEvent& event) noexcept;
	bool OnKeyReleased(KeyReleasedEvent& event) noexcept;
	bool OnKeyTyped(KeyTypedEvent& event) noexcept;

	bool OnMouseButtonPressed(MouseButtonPressedEvent& event) noexcept;
	bool OnMouseButtonReleased(MouseButtonReleasedEvent& event) noexcept;
	bool OnMouseMoved(MouseMovedEvent& event) noexcept;
	bool OnMouseScrolled(MouseScrolledEvent& event) noexcept;
	bool OnMouseEnter(MouseEnterEvent& event) noexcept;
	bool OnMouseLeave(MouseLeaveEvent& event) noexcept;
	bool OnMouseRawInput(MouseRawInputEvent& event) noexcept;

	template<typename T>
	static void PreventBufferOverflow(std::queue<T>& buffer) noexcept;

	template<typename T>
	static void PreventBufferOverflow(std::list<T>& buffer) noexcept;

public:
	bool MouseInWindow = false;
	int DeltaCarry = 0;

private:
	static constexpr uint32_t NumberOfKeys = 256u;
	static constexpr uint32_t BufferSize = 16u;

	std::bitset<NumberOfKeys> KeyStates;
	std::bitset<8> MouseStates;

	std::list<std::unique_ptr<Event>> KeyEventBuffer;
	std::queue<std::unique_ptr<Event>> MouseEventBuffer;
	std::queue<uint8> CharBuffer;
	std::queue<RawInputCoords> RawInputBuffer;

	bool RepeatEnabled = true;
	bool RawInputEnabled = true;
};