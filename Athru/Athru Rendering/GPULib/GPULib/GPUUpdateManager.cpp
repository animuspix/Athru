#include "GPUServiceCentre.h"
#include "UpdateShader.h"
#include "GPUUpdateManager.h"

GPUUpdateManager::GPUUpdateManager()
{
	Direct3D* d3dPttr = AthruGPU::GPUServiceCentre::AccessD3D();
	d3dContext = d3dPttr->GetDeviceContext();
	HWND handle = AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND();
	ID3D11Device* device = d3dPttr->GetDevice();
	physics = new UpdateShader(L"physicsSim.cso");
	cellDiff = new UpdateShader(L"critterSim.cso");
	ecology = new UpdateShader(L"ecoSim.cso");
}

GPUUpdateManager::~GPUUpdateManager()
{

}

void GPUUpdateManager::Update()
{
	//physics->Dispatch(d3dContext, x, y, z);
	//cellDiff->Dispatch(d3dContext, x, y, z);
	//ecology->Dispatch(d3dContext, x, y, z);
}
