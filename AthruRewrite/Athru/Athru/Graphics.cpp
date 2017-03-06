#include <intrin.h>
#include "MathIncludes.h"
#include "ServiceCentre.h"
#include "Graphics.h"

Graphics::Graphics(HWND windowHandle, Logger* logger)
{
	// Eww, eww, eww
	// Ask Adam about ways to refactor my memory manager so I can avoid this sort of thing
	d3D = DEBUG_NEW Direct3D(DISPLAY_WIDTH, DISPLAY_HEIGHT, VSYNC_ENABLED, windowHandle, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);

	// Create the camera object
	camera = new Camera();

	// Create a single boxecule
	boxecule = new Boxecule(d3D->GetDevice());

	// Create the shader manager
	renderManager = new RenderManager(d3D->GetDeviceContext(), d3D->GetDevice());
}

Graphics::~Graphics()
{
	delete d3D;
	d3D = nullptr;

	delete renderManager;
	renderManager = nullptr;

	delete boxecule;
	boxecule = nullptr;

	delete camera;
	camera = nullptr;
}

void Graphics::Frame()
{
	// Cache a local reference to the Input service
	Input* localInput = ServiceCentre::AccessInput();

	// Translate the camera with WASD
	if (localInput->IsKeyDown(87))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 10 * TimeStuff::deltaTime(), 0, 0), camera->GetRotation()));
	}

	if (localInput->IsKeyDown(65))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, 0, -10 * TimeStuff::deltaTime()), camera->GetRotation()));
	}

	if (localInput->IsKeyDown(83))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, -10 * TimeStuff::deltaTime(), 0, 0), camera->GetRotation()));
	}

	if (localInput->IsKeyDown(68))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, 0, 10 * TimeStuff::deltaTime()), camera->GetRotation()));
	}

	// Rotate the camera with mouse input
	camera->MouseLook(localInput);

	// Frustum culling here...
	//if ()
	//{
	//	renderManager->Register(boxecule);
	//}

	camera->RefreshViewMatrix();
	renderManager->Register(boxecule);

	// Record the time at this frame so we can calculate
	// [deltaTime]
	TimeStuff::timeAtLastFrame = std::chrono::steady_clock::now();;

	// No more updates, so pass control onto [Render()] and begin drawing to
	// the screen :)
	Render();
}

void Graphics::Render()
{
	d3D->BeginScene();
	renderManager->Render(d3D->GetWorldMatrix(), camera->GetViewMatrix(), d3D->GetPerspProjector());
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