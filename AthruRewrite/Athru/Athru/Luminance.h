#pragma once

enum class AVAILABLE_ILLUMINATION_TYPES
{
	DIRECTIONAL,
	POINT,
	SPOT
};

struct Luminance
{
	Luminance(float baseIntensity,
			  AVAILABLE_ILLUMINATION_TYPES baseIlluminationType) :
			  intensity(baseIntensity),
			  illuminationType(baseIlluminationType) {}

	// Specifying lighting direction means that the same
	// "Luminance" property can't flexibly represent either
	// directional/spot lights OR point lights without
	// wasting data; simpler and more efficient to assume
	// that directional lights are always projecting
	// along the local Z axis ("forward") while spot lights
	// always project negatively along the local Y axis
	// ("downward")

	float intensity;
	AVAILABLE_ILLUMINATION_TYPES illuminationType;
};

