#pragma once

#include "Direct3D.h"
#include "TextureManager.h"
#include "RenderManager.h"
#include "NormalBuilder.h"
#include "PlanarUnwrapper.h"
#include "UtilityServiceCentre.h"

namespace AthruRendering
{
	class RenderServiceCentre
	{
	public:
		RenderServiceCentre() = delete;
		~RenderServiceCentre() = delete;

		static void Init()
		{
			if (AthruUtilities::UtilityServiceCentre::AccessMemory() == nullptr)
			{
				// Allocation assumes Athru will use ten uncompressed RGBA
				// scene textures at most, then adds another 255 megabytes
				// beyond that to make sure no overruns occur during
				// runtime
				const eightByteUnsigned STARTING_HEAP = (GraphicsStuff::VOXEL_GRID_VOLUME * 40) + 255000000;
				AthruUtilities::UtilityServiceCentre::Init(STARTING_HEAP);
			}

			d3DPttr = DEBUG_NEW Direct3D(AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND());
			textureManagerPttr = new TextureManager(d3DPttr->GetDevice());
			renderManagerPttr = new RenderManager(textureManagerPttr->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::SCREEN_TEXTURE).asRenderedShaderResource);
		}

		static void DeInit()
		{
			textureManagerPttr->~TextureManager();
			textureManagerPttr = nullptr;

			renderManagerPttr->~RenderManager();
			renderManagerPttr = nullptr;

			delete d3DPttr;
			d3DPttr = nullptr;
		}

		static Direct3D* AccessD3D()
		{
			return d3DPttr;
		}

		static TextureManager* AccessTextureManager()
		{
			return textureManagerPttr;
		}

		static RenderManager* AccessRenderManager()
		{
			return renderManagerPttr;
		}

	private:
		static Direct3D* d3DPttr;
		static TextureManager* textureManagerPttr;
		static RenderManager* renderManagerPttr;
	};
}

