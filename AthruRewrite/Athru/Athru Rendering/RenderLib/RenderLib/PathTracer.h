#pragma once

#include <d3d11.h>
#include "ComputeShader.h"

class PathTracer : public ComputeShader
{
	public:
		PathTracer(LPCWSTR shaderFilePath);
		~PathTracer();
		void Dispatch(ID3D11DeviceContext* context);
};

