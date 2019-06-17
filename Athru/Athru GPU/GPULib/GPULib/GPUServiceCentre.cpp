#include "GPUServiceCentre.h"

Direct3D* AthruGPU::GPU::d3DPttr = nullptr;
GPUMessenger* AthruGPU::GPU::gpuMessengerPttr = nullptr;
Renderer* AthruGPU::GPU::rendererPttr = nullptr;
//GPUUpdateManager* AthruGPU::GPUServiceCentre::gpuUpdateManagerPttr = nullptr;
bool AthruGPU::GPU::internalInit = false;