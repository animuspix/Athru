#pragma once

#include <windows.h>
#include <directxmath.h>
#include "Application.h"

class Input
{
	enum class KEY_STATES
	{
		UP, // Key released/tapped
		DOWN, // Key active/held
		NUL // No messages received in the current frame
	};

	public:
		Input();
		~Input();

		// Set/check key states (where "true" represents "down"
		// and "false" represents "up")
		void KeyDown(u4Byte key);
		void KeyUp(u4Byte key);
		bool KeyHeld(u4Byte key);
		bool KeyTapped(u4Byte key);

		// Key reset function to avoid input states persisting between frames
		// (allows checking [keyUp] events for single-tap actions)
		void KeyReset();

		// Set/check the closing flag (tells the system to end the
		// game-loop and begin shutting down the application)
		void SetCloseFlag();
		bool GetCloseFlag();

		// Set/check the state of the left-mouse-button (where "true"
		// represents "down" or "pressed" and "false" represents "up"
		// or "released")
		void LeftMouseDown();
		void LeftMouseUp();
		bool GetLeftMouseDown();

		// Cache absolute mouse position within the window
		void CacheMousePos(float mouseX, float mouseY);

		// Retrieve mouse position within the window
		DirectX::XMFLOAT2 GetMousePos();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		KEY_STATES keys[256]; // Key states per-frame
		bool closeFlag;
		bool leftMouseDown;
		DirectX::XMFLOAT2 mousePos;
};

