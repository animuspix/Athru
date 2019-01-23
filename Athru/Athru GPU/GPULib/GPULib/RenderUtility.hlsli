
#ifndef MATERIALS_LINKED
	#include "Materials.hlsli"
#endif
#ifndef RENDER_UTILITY_CORE_LINKED
	#include "RenderUtilityCore.hlsli"
#endif
#ifndef PHILO_BUF_LINKED
	#include "PhiloBuf.hlsli"
#endif

// Write-allowed reference to the display texture
// (copied to the display each frame on rasterization, carries colors in [rgb] and
// filter values + pixel indices in [w])
RWTexture2D<float4> displayTex : register(u0);