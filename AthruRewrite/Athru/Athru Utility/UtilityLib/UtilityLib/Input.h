#pragma once

#include <windows.h>
#include <directxmath.h>
#include "Application.h"

class Input
{
	public:
		Input();
		~Input();

		void KeyDown(fourByteUnsigned);
		void KeyUp(fourByteUnsigned);
		bool IsKeyDown(fourByteUnsigned);

		// Cache absolute mouse position within the window
		void CacheMousePos(float mouseX, float mouseY);

		// Retrieve smoothed mouse position within the window
		DirectX::XMFLOAT2 GetMousePos();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		struct MouseData
		{
			struct PastMousePosition
			{
				PastMousePosition(DirectX::XMFLOAT2 basePos,
								  byteUnsigned baseAge) :
								  posRaw(basePos),
								  posWeighted(basePos),
								  age(baseAge) {}

				DirectX::XMFLOAT2 posRaw;
				DirectX::XMFLOAT2 posWeighted;
				byteUnsigned age;
			};

			MouseData() : mousePos(DirectX::XMFLOAT2(0, 0)),
						  pastPositions{ PastMousePosition(mousePos, 0), PastMousePosition(mousePos, 0),
										 PastMousePosition(mousePos, 0), PastMousePosition(mousePos, 0),
										 PastMousePosition(mousePos, 0), PastMousePosition(mousePos, 0),
										 PastMousePosition(mousePos, 0), PastMousePosition(mousePos, 0),
										 PastMousePosition(mousePos, 0), PastMousePosition(mousePos, 0) },
						  freshlyStoredPosCount(0),
						  moveRoughness(0.8f) {}

			DirectX::XMFLOAT2 mousePos;
			PastMousePosition pastPositions[10];
			byteUnsigned freshlyStoredPosCount;
			float moveRoughness;
		};

		bool keys[256];
		MouseData mouseInfo;
		DirectX::XMFLOAT2 smoothedMousePos;
};

