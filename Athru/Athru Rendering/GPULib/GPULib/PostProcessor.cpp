#include "GPUServiceCentre.h"
#include "PostProcessor.h"

PostProcessor::PostProcessor(LPCWSTR shaderFilePath) :
			   ComputeShader(AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice(),
			   			     AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND(),
			   			     shaderFilePath) {}

PostProcessor::~PostProcessor() {}

void PostProcessor::Dispatch(ID3D11DeviceContext* context,
							 ID3D11ShaderResourceView* giCalcBufferReadable)
{
	// Most of the shader inputs we expect to use were set earlier on in the frame,
	// so no need to set them again over here
	context->CSSetShader(shader, 0, 0);

	// Both views of the GI calculation buffer (readable/writable) access the same
	// resource; since we need to read from the buffer to average per-ray contributions
	// and finished writing to it in the previous pass, we should detach the writable
	// view before we attach the readable one
	ID3D11UnorderedAccessView* nullUAV = { NULL };
	context->CSSetUnorderedAccessViews(4, 1, &nullUAV, 0);
	context->CSSetShaderResources(1, 1, &giCalcBufferReadable);

	// This render-stage publishes to the screen, so we need to pass the
	// display texture along to the GPU
	ID3D11UnorderedAccessView* displayTexture = AthruGPU::GPUServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::SCREEN_TEXTURE).asWritableShaderResource;
	context->CSSetUnorderedAccessViews(2, 1, &displayTexture, 0);

	// We're only post-processing the most recent render pass, so deploy just
	// enought dispatch groups to fill one progressive slice of the display
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH, 1, 1);

	// We're going to need to access the writable GI calculation buffer view in the next frame,
	// but we can't do that if the readable view is still attached to the GPU; detach it
	// here
	ID3D11ShaderResourceView* nullSRV = { NULL };
	context->CSSetShaderResources(1, 1, &nullSRV);

	// We've finished initial rendering for this frame, so allow the presentation
	// pass to run by detaching the screen texture's unordered-access-view from the
	// GPU's compute shader state
	context->CSSetUnorderedAccessViews(2, 1, &nullUAV, 0);
}
