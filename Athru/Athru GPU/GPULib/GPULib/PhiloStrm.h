#pragma once

#include <DirectXMath.h>

// A GPU-side Philox stream representation
// Philox is a powerful parallel random-number-generator built around
// CTR-mode cryptography; it's described in the paper
// Parallel Random Numbers: As Easy as 1, 2, 3
// available here:
// http://www.thesalmons.org/john/random123/papers/random123sc11.pdf
// and a provided API, available here:
// http://www.deshawresearch.com/downloads/download_random123.cgi/
struct PhiloStrm
{
	DirectX::XMUINT4 ctr;
	DirectX::XMUINT2 key;
};