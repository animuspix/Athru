#include "GPUServiceCentre.h"

Direct3D* AthruGPU::GPUServiceCentre::d3DPttr = nullptr;
TextureManager* AthruGPU::GPUServiceCentre::textureManagerPttr = nullptr;
RenderManager* AthruGPU::GPUServiceCentre::renderManagerPttr = nullptr;
bool AthruGPU::GPUServiceCentre::internalInit = false;
GPURand* AthruGPU::GPUServiceCentre::gpuRand = nullptr;