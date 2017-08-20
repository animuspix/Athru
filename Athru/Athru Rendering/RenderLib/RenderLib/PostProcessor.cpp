#include "GPUServiceCentre.h"
#include "PostProcessor.h"

PostProcessor::PostProcessor(LPCWSTR shaderFilePath) :
			   ComputeShader(AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice(),
			   			     AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND(),
			   			     shaderFilePath) {}

PostProcessor::~PostProcessor() {}

void PostProcessor::Dispatch(ID3D11DeviceContext* context)
{
	// All the shader inputs we expect to use were set earlier on in the frame,
	// so no need to set them again over here
	context->CSSetShader(shader, 0, 0);

	// We're only post-processing the most recent render pass, so deploy the
	// same number of dispatch groups as in the primary ray-marcher
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH, 1, 1);

	// We've finished initial rendering for this frame, so allow the presentation
	// pass to run by detaching the screen texture's unordered-access-view from the
	// GPU's compute shader state
	ID3D11UnorderedAccessView* nullUAV = { NULL };
	context->CSSetUnorderedAccessViews(2, 1, &nullUAV, 0);
}
