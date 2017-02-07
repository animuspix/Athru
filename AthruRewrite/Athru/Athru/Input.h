#pragma once

class Input
{
	public:
		Input();
		~Input();

		void KeyDown(unsigned int);
		void KeyUp(unsigned int);
		bool IsKeyDown(unsigned int);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		bool keys[256];
};

