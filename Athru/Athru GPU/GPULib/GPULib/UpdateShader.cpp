#include "GPUServiceCentre.h"
#include "UpdateShader.h"

UpdateShader::UpdateShader(LPCWSTR shaderFilePath) :
						   ComputeShader(AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice(),
										 AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND(),
										 shaderFilePath) {}

UpdateShader::~UpdateShader()
{
}

void UpdateShader::Dispatch(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
							fourByteUnsigned threadsX,
							fourByteUnsigned threadsY,
							fourByteUnsigned threadsZ)
{
	context->CSSetShader(shader.Get(), 0, 0);
	context->Dispatch(threadsX, threadsY, threadsZ);
}


