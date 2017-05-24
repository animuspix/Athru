// Athru utility services
#include "UtilityServiceCentre.h"

// Athru rendering services
#include "RenderServiceCentre.h"

// The Direct3D handler class
#include "Direct3D.h"

// Pipeline shaders
#include "Rasterizer.h"
#include "PostProcessor.h"

// Pre-processing/Scene-sampling shaders
#include "SystemSampler.h"
#include "PlanetSampler.h"
#include "SceneIlluminator.h"

// Generic objects relevant to [this]
#include "VoxelGrid.h"
#include "Camera.h"
#include "Planet.h"
#include "Star.h"

// The header containing declarations used by [this]
#include "RenderManager.h"

RenderManager::RenderManager(ID3D11ShaderResourceView* postProcessShaderResource)
{
	// Cache a class-scope reference to the Direct3D handler class
	d3D = AthruRendering::RenderServiceCentre::AccessD3D();

	// Cache a class-scope reference to the Direct3D rendering context
	d3dContext = d3D->GetDeviceContext();

	// Cache a local reference to the Direct3D rendering device
	ID3D11Device* d3dDevice = d3D->GetDevice();

	// Cache a local copy of the window handle
	HWND localWindowHandle = AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND();

	// Construct the voxel grid
	voxGrid = new VoxelGrid(d3dDevice);

	// Cache class-scope references to scene textures
	AthruTexture3D sceneColorTexture = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES::SCENE_COLOR_TEXTURE);
	AthruTexture3D sceneNormalTexture = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES::SCENE_NORMAL_TEXTURE);
	AthruTexture3D scenePBRTexture = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES::SCENE_PBR_TEXTURE);
	AthruTexture3D sceneEmissivityTexture = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES::SCENE_EMISSIVITY_TEXTURE);

	// Cache class-scope references to effect masks
	AthruTexture2D blurMask = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::BLUR_MASK);
	AthruTexture2D brightenMask = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::BRIGHTEN_MASK);
	AthruTexture2D drugsMask = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::DRUGS_MASK);

	// Construct the shaders used by [this]

	// Construct the object shader + the post-processing shader
	rasterizer = new Rasterizer(d3dDevice, localWindowHandle, L"VertPlotter.cso", L"Colorizer.cso",
								sceneColorTexture.asRenderedShaderResource);

	postProcessor = new PostProcessor(d3dDevice, localWindowHandle, L"PostVertPlotter.cso", L"PostColorizer.cso",
									  postProcessShaderResource);

	// Construct the system sampling shader
	systemSampler = new SystemSampler(d3dDevice, localWindowHandle, L"SystemSampler.cso",
									  sceneColorTexture.asWritableComputeShaderResource, sceneNormalTexture.asWritableComputeShaderResource,
									  scenePBRTexture.asWritableComputeShaderResource, sceneEmissivityTexture.asWritableComputeShaderResource);

	// Construct the planetary archetype sampling shaders
	Heightfield archetypeZeroHeightfield = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetPlanetaryHeightfield(AVAILABLE_PLANETARY_HEIGHTFIELDS::ARCHETYPE_ZERO);
	planetSamplers[(byteUnsigned)AVAILABLE_PLANET_ARCHETYPES::ARCHETYPE_0] = new PlanetSampler(d3dDevice, localWindowHandle, L"PlanetSampler.cso",
																							   archetypeZeroHeightfield.upperHemisphere.asComputeShaderResource,
																							   archetypeZeroHeightfield.lowerHemisphere.asComputeShaderResource,
																							   sceneColorTexture.asWritableComputeShaderResource,
																							   sceneNormalTexture.asWritableComputeShaderResource,
																							   scenePBRTexture.asWritableComputeShaderResource,
																							   sceneEmissivityTexture.asWritableComputeShaderResource);

	// Construct the scene lighting shader
	sceneIlluminator = new SceneIlluminator(d3dDevice, localWindowHandle, L"SceneIlluminator.cso",
											sceneColorTexture.asWritableComputeShaderResource, sceneNormalTexture.asComputeShaderResource,
											scenePBRTexture.asComputeShaderResource, sceneEmissivityTexture.asComputeShaderResource);
}

RenderManager::~RenderManager()
{
	// Delete heap-allocated data within the object shader
	rasterizer->~Rasterizer();
	rasterizer = nullptr;

	// Delete heap-allocated data within the post-processing shader
	postProcessor->~PostProcessor();
	postProcessor = nullptr;
}

void RenderManager::Render(Camera* mainCamera, DirectX::XMVECTOR systemPos,
						   Planet** planets, Star* star)
{
	// Cache a local reference to the camera's viewfinder
	// (screen rect)
	ScreenRect* viewFinderPttr = mainCamera->GetViewFinder();

	// Prepare DirectX for scene rendering
	d3D->BeginScene(viewFinderPttr->GetRenderTarget());

	// Cache temporary copies of the world, view, and perspective
	// projection matrices
	DirectX::XMMATRIX world = d3D->GetWorldMatrix();
	DirectX::XMMATRIX view = mainCamera->GetViewMatrix();
	DirectX::XMMATRIX perspProjection = d3D->GetPerspProjector();

	// Compute scene data for the current main-camera position
	this->PreProcess(mainCamera->GetTranslation(), systemPos,
					 mainCamera->GetLookData().viewDirNormal,
					 planets, star);

	// Render scene to the screen texture
	this->RenderScene(world, view, perspProjection);

	// Prepare DirectX for post-processing
	d3D->BeginPost();

	// Post-process
	this->PostProcess(viewFinderPttr, world, view, perspProjection);

	// Present the post-processed scene to the display
	d3D->EndScene();
}

void RenderManager::PreProcess(DirectX::XMVECTOR cameraPos, DirectX::XMVECTOR systemPos,
							   DirectX::XMVECTOR cameraViewVector,
							   Planet** planets, Star* star)
{
	// Calculate distances from the camera to each planet in the system

	DirectX::XMVECTOR planetAGlobalPos = _mm_add_ps(planets[0]->FetchStellarOffset(), systemPos);
	DirectX::XMVECTOR cameraPosDiffPlanetA = _mm_sub_ps(planetAGlobalPos, cameraPos);
	float cameraDistPlanetA = _mm_cvtss_f32(DirectX::XMVector3Length(cameraPosDiffPlanetA));

	DirectX::XMVECTOR planetBGlobalPos = _mm_add_ps(planets[1]->FetchStellarOffset(), systemPos);
	DirectX::XMVECTOR cameraPosDiffPlanetB = _mm_sub_ps(planetBGlobalPos, cameraPos);
	float cameraDistPlanetB = _mm_cvtss_f32(DirectX::XMVector3Length(cameraPosDiffPlanetB));

	DirectX::XMVECTOR planetCGlobalPos = _mm_add_ps(planets[2]->FetchStellarOffset(), systemPos);
	DirectX::XMVECTOR cameraPosDiffPlanetC = _mm_sub_ps(planetCGlobalPos, cameraPos);
	float cameraDistPlanetC = _mm_cvtss_f32(DirectX::XMVector3Length(cameraPosDiffPlanetC));

	// If the camera is inside the minimum planetary draw-distance, call the relevant planetary
	// samplers; otherwise, call the system sampler with the known planetary
	// positions + colors

	if (cameraDistPlanetA > GraphicsStuff::MIN_PLANET_SAMPLING_DIST &&
		cameraDistPlanetB > GraphicsStuff::MIN_PLANET_SAMPLING_DIST &&
		cameraDistPlanetC > GraphicsStuff::MIN_PLANET_SAMPLING_DIST)
	{
		systemSampler->Dispatch(d3dContext, cameraPos, systemPos, star->GetColor(),
								planetAGlobalPos, planetBGlobalPos, planetCGlobalPos,
								planets[0]->GetAvgColor(), planets[1]->GetAvgColor(),
								planets[2]->GetAvgColor());
	}

	else
	{
		// No weighting for now, so just render everything with the zeroth (read: only) planetary sampler
		if (cameraDistPlanetA > cameraDistPlanetB &&
			cameraDistPlanetA > cameraDistPlanetC)
		{
			//for (byteUnsigned i = 0; i < (byteUnsigned)(AVAILABLE_PLANET_ARCHETYPES::NULL_ARCHETYPE); i += 1)
			//{
			//	planetSamplers[i]->Dispatch(d3dContext);
			//}

			planetSamplers[0]->Dispatch(d3dContext, planetAGlobalPos, cameraPos,
										planets[0]->GetAvgColor(), systemPos);
		}

		else if (cameraDistPlanetB > cameraDistPlanetA &&
				 cameraDistPlanetB > cameraDistPlanetC)
		{
			//for (byteUnsigned i = 0; i < (byteUnsigned)(AVAILABLE_PLANET_ARCHETYPES::NULL_ARCHETYPE); i += 1)
			//{
			//	planetSamplers[i]->Dispatch(d3dContext);
			//}

			planetSamplers[0]->Dispatch(d3dContext, planetBGlobalPos, cameraPos,
										planets[1]->GetAvgColor(), systemPos);
		}

		else
		{
			//for (byteUnsigned i = 0; i < (byteUnsigned)(AVAILABLE_PLANET_ARCHETYPES::NULL_ARCHETYPE); i += 1)
			//{
			//	planetSamplers[i]->Dispatch(d3dContext);
			//}

			planetSamplers[0]->Dispatch(d3dContext, planetCGlobalPos, cameraPos,
										planets[2]->GetAvgColor(), systemPos);
		}
	}

	// Use the known PBR/Emissivity data to calculate approximate global illumination for
	// the scene
	//sceneIlluminator->Dispatch(d3dContext, cameraViewVector);
}

void RenderManager::RenderScene(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Pass the voxel grid onto the GPU, then render it with the object shader
	voxGrid->PassToGPU(d3dContext);
	rasterizer->Render(d3dContext, world, view, projection);
}

void RenderManager::PostProcess(ScreenRect* screenRect,
								DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Pass the rect onto the GPU, then render it with the post-processing shader
	screenRect->PassToGPU(d3dContext);
	postProcessor->Render(d3dContext, false, false, false);
}

Direct3D* RenderManager::GetD3D()
{
	return d3D;
}

// Push constructions for this class through Athru's custom allocator
void* RenderManager::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<RenderManager>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void RenderManager::operator delete(void* target)
{
	return;
}
