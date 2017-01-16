#include "ServiceCentre.h"
#include "Input.h"

Input::Input()
{
	// Un-press every key in on the keyboard
	for (twoByteUnsigned keySetter = 0; keySetter < 256; keySetter += 1)
	{
		keys[keySetter] = false;
	}
}

Input::~Input()
{
}

void Input::KeyDown(unsigned int input)
{
	// If a key is pressed then save that state in the key array.
	keys[input] = true;
	return;
}

void Input::KeyUp(unsigned int input)
{
	// If a key is released then clear that state in the key array.
	keys[input] = false;
	return;
}

bool Input::IsKeyDown(unsigned int key)
{
	// Return what state the key is in (pressed/not pressed).
	return keys[key];
}

// Push constructions for this class through Athru's custom allocator
void* Input::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::Instance().AccessMemory();
	return allocator->AlignedAlloc((fourByteUnsigned)size, 4, false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Input::operator delete(void* target)
{
	return;
}
