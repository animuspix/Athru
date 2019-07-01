#pragma once

#include <directxmath.h>
#include "UtilityServiceCentre.h"

struct PixHistory
{
	DirectX::XMVECTOR currSampleAccum; // Current + incomplete set of [NUM_AA_SAMPLES] samples/filter-coefficients
	DirectX::XMUINT4 sampleCount; // Cumulative number of samples at the start/end of each frame
};