#include "ServiceCentre.h"
#include "Graphics.h"

Graphics::Graphics(int screenWidth, int screenHeight, HWND hwnd)
{
	d3D = Direct3D(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
}

Graphics::~Graphics()
{
}

void Graphics::Frame()
{
	Render();
}

void Graphics::Render()
{
	// d3D.BeginScene(0.5f, 0.5f, 0.5f, 0.2f);
	// d3D.EndScene();
}

// Push constructions for this class through Athru's custom allocator
void* Graphics::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc((fourByteUnsigned)size, 4, false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Graphics::operator delete(void* target)
{
	return;
}