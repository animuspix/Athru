#include "ServiceCentre.h"
#include "Graphics.h"

Graphics::Graphics(int screenWidth, int screenHeight, HWND windowHandle, Logger* logger)
{
	// Eww, eww, eww
	// Ask Adam about ways to refactor my memory manager so I can avoid this sort of thing
	d3D = DEBUG_NEW Direct3D(screenWidth, screenHeight, VSYNC_ENABLED, windowHandle, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);

	// Create the camera object
	camera = new Camera();

	// Create a Triangle
	triangle = new Triangle(d3D->GetDevice());

	// Create the shader manager
	shaderManager = new Shaders(d3D->GetDevice(), windowHandle);
}

Graphics::~Graphics()
{
	delete d3D;
	d3D = nullptr;

	delete shaderManager;
	shaderManager = nullptr;

	delete triangle;
	triangle = nullptr;

	delete camera;
	camera = nullptr;
}

void Graphics::Frame()
{
	Render();
}

void Graphics::Render()
{
	d3D->BeginScene();

	// Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	triangle->Render(d3D->GetDeviceContext());

	// Render the model using [VertPlotter] and [Colorizer]
	shaderManager->Render(d3D->GetDeviceContext(), d3D->GetWorldMatrix(), camera->GetViewMatrix(), d3D->GetPerspProjector());

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