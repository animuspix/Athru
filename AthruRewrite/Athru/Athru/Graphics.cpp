#include <intrin.h>
#include "ServiceCentre.h"
#include "Camera.h"
#include "Graphics.h"

Graphics::Graphics(HWND windowHandle)
{
	d3D = DEBUG_NEW Direct3D(windowHandle);
}

void Graphics::FetchDependencies()
{
	// Retrieve a pointer to the render manager from the service centre
	renderManagerPttr = ServiceCentre::AccessRenderManager();

	// Retrieve a pointer to the scene manager from the service centre
	sceneManagerPttr = ServiceCentre::AccessSceneManager();

	// Create the camera object (involves fetching textures from the texture manager,
	// which can't be created before [this] because it needs to access [d3D] at
	// construction)
	camera = new Camera(d3D->GetDevice());
}

Graphics::~Graphics()
{
	// Flush any pipeline data associated with [this]
	d3D->GetDeviceContext()->ClearState();
	d3D->GetDeviceContext()->Flush();

	// Call the camera object's destructor (although most of it's
	// data is tracked and controlled by [StackAllocator], some
	// stuff (e.g. the render target associated with [viewfinder])
	// is controlled by the system and must be cleared manually)
	camera->~Camera();

	// Release the Direct3D handler class
	delete d3D;
	d3D = nullptr;
}

void Graphics::Frame()
{
	ServiceCentre::AccessLogger()->Log("Logging CPU-side FPS", Logger::DESTINATIONS::CONSOLE);
	ServiceCentre::AccessLogger()->Log(TimeStuff::FPS(), Logger::DESTINATIONS::CONSOLE);

	// Cache a local reference to the Input service
	Input* localInput = ServiceCentre::AccessInput();

	// Translate the camera with WASD
	float speed = 10;
	if (localInput->IsKeyDown(87))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, speed * TimeStuff::deltaTime(), 0, 0), camera->GetRotationQuaternion()));
	}

	if (localInput->IsKeyDown(65))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, 0, (speed * TimeStuff::deltaTime()) * -1), camera->GetRotationQuaternion()));
	}

	if (localInput->IsKeyDown(83))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, (speed * TimeStuff::deltaTime()) * -1, 0, 0), camera->GetRotationQuaternion()));
	}

	if (localInput->IsKeyDown(68))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, 0, speed * TimeStuff::deltaTime()), camera->GetRotationQuaternion()));
	}

	if (localInput->IsKeyDown(32))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, speed * TimeStuff::deltaTime(), 0), camera->GetRotationQuaternion()));
	}

	if (localInput->IsKeyDown(17))
	{
		camera->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, (speed * TimeStuff::deltaTime()) * -1, 0), camera->GetRotationQuaternion()));
	}

	// Rotate the camera with mouse input
	camera->MouseLook(localInput);

	// Update the camera's view matrix to
	// reflect the translation + rotation above
	camera->RefreshViewData();

	// Update the scene (generate terrain/organisms, update organism statuses,
	// simulate physics for visible areas, etc.)
	sceneManagerPttr->Update(camera->GetTranslation());

	// Pass the boxecules currently in the scene along to the
	// render manager
	renderManagerPttr->Prepare(sceneManagerPttr->GetChunks(), camera, sceneManagerPttr->GetBoxeculeDensity());

	// Record the time at this frame so we can calculate
	// [deltaTime]
	TimeStuff::timeAtLastFrame = std::chrono::steady_clock::now();

	// No more updates, so pass control onto [Render()] and begin drawing to
	// the screen :)
	Render();
}

void Graphics::Render()
{
	// Cache a reference to the camera's viewfinder (the screen rect)
	AthruRect* viewFinderPttr = camera->GetViewFinder();

	// Cache references to the world matrix, the camera's view matrix,
	// and the perspective projection matrix
	DirectX::XMMATRIX& world = d3D->GetWorldMatrix();
	DirectX::XMMATRIX& view = camera->GetViewMatrix();
	DirectX::XMMATRIX& perspProjector = d3D->GetPerspProjector();

	// Perform initial render
	d3D->BeginScene(viewFinderPttr->GetRenderTarget());
	renderManagerPttr->Render(camera, world, view, perspProjector);

	// Perform post-processing through the screen rect (the camera's viewfinder)
	d3D->BeginPost();
	renderManagerPttr->PostProcess(viewFinderPttr, world, view, perspProjector);

	// Display the post-processed data by pushing the camera's viewfinder to the screen
	d3D->EndScene();
}

Direct3D* Graphics::GetD3D()
{
	return d3D;
}

// Push constructions for this class through Athru's custom allocator
void* Graphics::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Graphics>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Graphics::operator delete(void* target)
{
	return;
}