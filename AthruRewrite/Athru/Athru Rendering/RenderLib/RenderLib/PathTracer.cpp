#include "GPUServiceCentre.h"
#include "PathTracer.h"

PathTracer::PathTracer(LPCWSTR shaderFilePath) :
			ComputeShader(AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice(),
						  AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND(),
						  shaderFilePath) {}

PathTracer::~PathTracer() {}

void PathTracer::Dispatch(ID3D11DeviceContext* context)
{
	// All the shader inputs we expect to use were already set by the ray-marcher,
	// so no need to set them again over here
	context->CSSetShader(shader, 0, 0);

	// We can't effectively deploy more than 64 threads in each group, so we're
	// making up the difference by deploying four times as many work groups and
	// integrating as if we were tracing 256 (4 * 64) simultaneous secondary
	// rays (iirc 256 is the upper limit for sensible path-tracing sample
	// counts)
	context->Dispatch(GraphicsStuff::MAX_TRACE_COUNT / 4, 16, 1);
}
