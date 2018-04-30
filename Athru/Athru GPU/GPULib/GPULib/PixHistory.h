#pragma once

#include <directxmath.h>
#include "UtilityServiceCentre.h"

struct PixHistory
{
	DirectX::XMVECTOR sampleAccum; // Accumulated samples
	DirectX::XMVECTOR filtAccum; // Accumulated filter coefficients
	DirectX::XMFLOAT3 sampleSet[GraphicsStuff::NUM_AA_SAMPLES]; // Discretized sample-set
	float filtSet[GraphicsStuff::NUM_AA_SAMPLES]; // Discretized set of filter coefficients
	DirectX::XMUINT4 sampleCount; // Cumulative number of samples at the start/end of each frame
	DirectX::XMVECTOR incidentLight; // Incidental samples collected from light paths bouncing
									 // off the scene before reaching the lens; exists because most
									 // light sub-paths enter the camera through pseudo-random sensors
									 // rather than reaching the sensor (i.e. thread) associated with
									 // their corresponding camera sub-path
									 // Separate to [sampleAccum] to prevent simultaneous writes during
									 // path filtering/anti-aliasing
};