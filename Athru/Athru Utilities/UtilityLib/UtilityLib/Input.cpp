#include "UtilityServiceCentre.h"
#include "Input.h"

Input::Input()
{
	// Leave keys undefined until Athru starts processing Windows messages
	for (u2Byte keySetter = 0; keySetter < 256; keySetter += 1)
	{
		keys[keySetter] = KEY_STATES::NUL;
	}

	// Initialise the flag storing whether or not the application
	// has received a window-close message from the OS
	closeFlag = false;

	// Initialise mouse information
	mousePos = DirectX::XMFLOAT2((float)(GraphicsStuff::DISPLAY_WIDTH / 2),
								 (float)(GraphicsStuff::DISPLAY_HEIGHT / 2));

	leftMouseDown = false;
}

Input::~Input() {}

void Input::KeyDown(u4Byte input)
{
	// If a key is pressed then save that state in the key array.
	keys[input] = KEY_STATES::DOWN;
}

void Input::KeyUp(u4Byte input)
{
	// If a key is released then clear that state in the key array.
	keys[input] = KEY_STATES::UP;
}

bool Input::KeyHeld(u4Byte key)
{
	return keys[key] == KEY_STATES::DOWN;
}

bool Input::KeyTapped(u4Byte key)
{
	return keys[key] == KEY_STATES::UP;
}

void Input::KeyReset()
{
	for (u4Byte i = 0; i < 256; i += 1)
	{
		keys[i] = (keys[i] == KEY_STATES::UP) ? KEY_STATES::NUL : keys[i];
	}
}

void Input::SetCloseFlag()
{
	closeFlag = true;
}

bool Input::GetCloseFlag()
{
	return closeFlag;
}

void Input::LeftMouseDown()
{
	leftMouseDown = true;
}

void Input::LeftMouseUp()
{
	leftMouseDown = false;
}

bool Input::GetLeftMouseDown()
{
	return leftMouseDown;
}

void Input::CacheMousePos(float mouseX, float mouseY)
{
	mousePos.x = mouseX;
	mousePos.y = mouseY;
}

DirectX::XMFLOAT2 Input::GetMousePos()
{
	return mousePos;
}

// Push constructions for this class through Athru's custom allocator
void* Input::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<Input>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Input::operator delete(void* target)
{
	return;
}
