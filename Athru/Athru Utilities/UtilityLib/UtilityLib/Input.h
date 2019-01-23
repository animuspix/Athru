#pragma once

#include <windows.h>
#include <directxmath.h>
#include "Application.h"

class Input
{
	public:
		Input();
		~Input();

		// Set/check key states (where "true" represents "down"
		// and "false" represents "up")
		void KeyDown(u4Byte);
		void KeyUp(u4Byte);
		bool IsKeyDown(u4Byte);

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

		// Retrieve smoothed mouse position within the window
		DirectX::XMFLOAT2 GetMousePos();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		bool keys[256];
		bool closeFlag;
		bool leftMouseDown;
		DirectX::XMFLOAT2 mousePos;
};

