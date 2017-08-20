#pragma once

#include <directxmath.h>

struct TracePoint
{
	DirectX::XMVECTOR coord;
	DirectX::XMVECTOR rgbaSrc;
	DirectX::XMUINT4 figID;
	DirectX::XMUINT4 isValid;
	DirectX::XMVECTOR rgbaGI;
};