#include "DemoPlanetSampler.h"

DemoPlanetSampler::DemoPlanetSampler(ID3D11Device* device, HWND windowHandle, 
									 LPCWSTR shaderFilePath) :
									 ComputeShader(device, windowHandle, 
												   shaderFilePath)
{ 

}

DemoPlanetSampler::~DemoPlanetSampler()
{
}
