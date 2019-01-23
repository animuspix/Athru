#include "GPUServiceCentre.h"
#include "UpdateShader.h"

UpdateShader::UpdateShader(LPCWSTR shaderFilePath) :
						   ComputeShader(AthruGPU::GPU::AccessD3D()->GetDevice(),
										 AthruCore::Utility::AccessApp()->GetHWND(),
										 shaderFilePath) {}

UpdateShader::~UpdateShader()
{
}

void UpdateShader::Dispatch(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
							u4Byte threadsX,
							u4Byte threadsY,
							u4Byte threadsZ)
{
	context->CSSetShader(shader.Get(), 0, 0);
	context->Dispatch(threadsX, threadsY, threadsZ);
}


