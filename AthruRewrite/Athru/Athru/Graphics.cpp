#include "ServiceCentre.h"
#include "Graphics.h"

Graphics::Graphics(int screenWidth, int screenHeight, HWND hwnd, StackAllocator* allocator)
{
	// Eww, eww, eww
	// Ask Adam about ways to refactor my memory manager so I can avoid this sort of thing
	d3D = DEBUG_NEW Direct3D(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
}

Graphics::~Graphics()
{
	delete d3D;
	d3D = nullptr;
}

void Graphics::Frame()
{
	Render();
}

void Graphics::Render()
{
	d3D->BeginScene();
	d3D->EndScene();
}

// Push constructions for this class through Athru's custom allocator
void* Graphics::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc((fourByteUnsigned)size, (byteUnsigned)std::alignment_of<Graphics>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Graphics::operator delete(void* target)
{
	return;
}