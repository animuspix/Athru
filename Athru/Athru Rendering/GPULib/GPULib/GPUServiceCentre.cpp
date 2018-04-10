#include "GPUServiceCentre.h"

Direct3D* AthruGPU::GPUServiceCentre::d3DPttr = nullptr;
GPUMessenger* AthruGPU::GPUServiceCentre::gpuMessengerPttr = nullptr;
RenderManager* AthruGPU::GPUServiceCentre::renderManagerPttr = nullptr;
GPUUpdateManager* AthruGPU::GPUServiceCentre::gpuUpdateManagerPttr = nullptr;
bool AthruGPU::GPUServiceCentre::internalInit = false;
GPURand* AthruGPU::GPUServiceCentre::gpuRand = nullptr;