#include "ServiceCentre.h"
#include "Typedefs.h"
#include "Input.h"

Input::Input() : Service("Input")
{
	// Un-press every key in on the keyboard
	for (twoByteUnsigned keySetter = 0; keySetter < 256; keySetter += 1)
	{
		keys[keySetter] = false;
	}
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

Input::~Input()
{
}
