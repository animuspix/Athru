#include "stdafx.h"
#include "AthruGlobals.h"
#include "Input.h"

Input::Input()
{
	// Un-press every key in on the keyboard
	for (twoByteUnsigned keySetter = 0; keySetter < 256; keySetter += 1)
	{
		keys[keySetter] = false;
	}

	// Initialise mouse information
	mouseInfo = MouseData();
	smoothedMousePos = DirectX::XMFLOAT2(0, 0);
}

Input::~Input()
{
}

void Input::KeyDown(fourByteUnsigned input)
{
	// If a key is pressed then save that state in the key array.
	keys[input] = true;
	return;
}

void Input::KeyUp(fourByteUnsigned input)
{
	// If a key is released then clear that state in the key array.
	keys[input] = false;
	return;
}

bool Input::IsKeyDown(fourByteUnsigned key)
{
	// Return what state the key is in (pressed/not pressed).
	return keys[key];
}

void Input::CacheMousePos(float mouseX, float mouseY)
{
	// Minor bit-magic here to check whether subtracting one from the
	// index used with [mouseInfo.pastPositions] is safe, and to check
	// whether the counter for "fresh" mouse positions should be reset
	// to zero

	// Move the last-used mouse position into the array of past positions,
	// then mark it's age as [0] since it's only just been cached
	bool freshCountsExist = mouseInfo.freshlyStoredPosCount != 0;
	mouseInfo.pastPositions[mouseInfo.freshlyStoredPosCount - (byteUnsigned)freshCountsExist].posRaw.x = mouseInfo.mousePos.x;
	mouseInfo.pastPositions[mouseInfo.freshlyStoredPosCount - (byteUnsigned)freshCountsExist].posRaw.y = mouseInfo.mousePos.y;
	mouseInfo.pastPositions[mouseInfo.freshlyStoredPosCount - (byteUnsigned)freshCountsExist].age = 0;
	mouseInfo.freshlyStoredPosCount += 1;
	mouseInfo.freshlyStoredPosCount *= mouseInfo.freshlyStoredPosCount != 11;

	// Weight stored mouse positions according to how recently they
	// were cached, then increment the age of each position by [1]
	// and increment the sums of the [x] and [y] coords (used in the
	// filtering stage) by the values in the [x] and [y] coords at
	// each step
	for (byteUnsigned i = 0; i < 10; i += 2)
	{
		MouseData::PastMousePosition& positionA = mouseInfo.pastPositions[i];
		positionA.posWeighted.x = (float)((double)positionA.posRaw.x * pow(mouseInfo.moveRoughness, positionA.age));
		positionA.posWeighted.y = (float)((double)positionA.posRaw.y * pow(mouseInfo.moveRoughness, positionA.age));
		positionA.age += 1;

		MouseData::PastMousePosition& positionB = mouseInfo.pastPositions[i + 1];
		positionB.posWeighted.x = (float)((double)positionB.posRaw.x * pow(mouseInfo.moveRoughness, positionB.age));
		positionB.posWeighted.y = (float)((double)positionB.posRaw.y * pow(mouseInfo.moveRoughness, positionB.age));
		positionB.age += 1;

		smoothedMousePos.x += (positionA.posWeighted.x + positionB.posWeighted.x);
		smoothedMousePos.y += (positionA.posWeighted.y + positionB.posWeighted.y);
	}

	// Update the smoothed mouse position by multiplying the stored sums by the inverse of the
	// number of stored past positions (multiplication is faster than division in C++)
	smoothedMousePos.x *= 0.1f;
	smoothedMousePos.y *= 0.1f;

	// Update the stored mouse position (raw) with the given values
	mouseInfo.mousePos.x = mouseX;
	mouseInfo.mousePos.y = mouseY;
}

DirectX::XMFLOAT2 Input::GetMousePos()
{
	//return smoothedMousePos;
	return mouseInfo.mousePos;
}

// Push constructions for this class through Athru's custom allocator
void* Input::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Input>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Input::operator delete(void* target)
{
	return;
}
