#include "GPUServiceCentre.h"

Direct3D* AthruGPU::GPU::d3DPttr = nullptr;
GPUMessenger* AthruGPU::GPU::gpuMessengerPttr = nullptr;
Renderer* AthruGPU::GPU::rendererPttr = nullptr;
bool AthruGPU::GPU::internalInit = false;