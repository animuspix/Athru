#include "Input.h"

Input::Input()
{
	short keySetter;

	// Un-press every key in on the keyboard
	for (keySetter = 0; keySetter < 256; keySetter += 1)
	{
		m_keys[keySetter] = false;
	}
}

void Input::KeyDown(unsigned int input)
{
	// If a key is pressed then save that state in the key array.
	m_keys[input] = true;
	return;
}


void Input::KeyUp(unsigned int input)
{
	// If a key is released then clear that state in the key array.
	m_keys[input] = false;
	return;
}


bool Input::IsKeyDown(unsigned int key)
{
	// Return what state the key is in (pressed/not pressed).
	return m_keys[key];
}

Input::~Input()
{
}
