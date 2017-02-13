#pragma once

class Input
{
	public:
		Input();
		~Input();

		void KeyDown(fourByteUnsigned);
		void KeyUp(fourByteUnsigned);
		bool IsKeyDown(fourByteUnsigned);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		bool keys[256];
};

