#include "GPUServiceCentre.h"

Direct3D* AthruGPU::GPUServiceCentre::d3DPttr = nullptr;
GPUMessenger* AthruGPU::GPUServiceCentre::gpuMessengerPttr = nullptr;
FigureRaster* AthruGPU::GPUServiceCentre::rasterPttr = nullptr;
Renderer* AthruGPU::GPUServiceCentre::rendererPttr = nullptr;
//GPUUpdateManager* AthruGPU::GPUServiceCentre::gpuUpdateManagerPttr = nullptr;
bool AthruGPU::GPUServiceCentre::internalInit = false;
GPURand* AthruGPU::GPUServiceCentre::gpuRand = nullptr;