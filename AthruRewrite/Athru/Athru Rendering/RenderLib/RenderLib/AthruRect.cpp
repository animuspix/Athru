#include "UtilityServiceCentre.h"
#include "NormalBuilder.h"
#include "PlanarUnwrapper.h"
#include "AthruRect.h"

AthruRect::AthruRect(ID3D11Device* d3dDevice)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

	// Create a render-target-view from the screen texture
	//d3dDevice->CreateRenderTargetView(TextureManager()->GetInternalTexture2D(AVAILABLE_INTERNAL_TEXTURES::SCREEN_TEXTURE).raw, nullptr, &renderTarget);

	// Cache the color associated with [this]
	DirectX::XMFLOAT4 vertColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Cache the roughness and reflectiveness associated with [this]
	float vertRoughness = 1.0f;
	float vertReflectiveness = 1.0f;

	// Store min bounding cube position, max bounding cube position, and the difference (range) between them
	DirectX::XMFLOAT4 minBoundingPos = DirectX::XMFLOAT4(-1.0f, -1.0f, 0.1f, 1.0f);
	DirectX::XMFLOAT4 maxBoundingPos = DirectX::XMFLOAT4(1.0f, 1.0f, 0.1f, 1.0f);
	DirectX::XMFLOAT4 boundingRange = DirectX::XMFLOAT4(maxBoundingPos.x - minBoundingPos.x,
														maxBoundingPos.y - minBoundingPos.y,
														maxBoundingPos.z - minBoundingPos.z, 1);

	// Generate + cache vertex positions
	DirectX::XMFLOAT4 vert0Pos = DirectX::XMFLOAT4(-1.0f, 1.0f, 0.1f, 1.0f); // Front plane, upper left (v0)
	DirectX::XMFLOAT4 vert1Pos = DirectX::XMFLOAT4(1.0f, 1.0f, 0.1f, 1.0f); // Front plane, upper right (v1)
	DirectX::XMFLOAT4 vert2Pos = DirectX::XMFLOAT4(-1.0f, -1.0f, 0.1f, 1.0f); // Front plane, lower left (v2)
	DirectX::XMFLOAT4 vert3Pos = DirectX::XMFLOAT4(1.0f, -1.0f, 0.1f, 1.0f); // Front plane, lower right (v3)

	// Initialise vertices
	Vertex vertices[4] = { Vertex(vert0Pos, // Upper left (v0)
								  vert0Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert0Pos),
								  NormalBuilder::BuildNormal(vert0Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert0Pos),
								  vertRoughness,
								  vertReflectiveness),

						   Vertex(vert1Pos, // Upper right (v1)
								  vert1Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert1Pos),
								  NormalBuilder::BuildNormal(vert0Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert1Pos),
								  vertRoughness,
								  vertReflectiveness),

						   Vertex(vert2Pos, // Lower left (v2)
								  vert2Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert2Pos),
								  NormalBuilder::BuildNormal(vert0Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert2Pos),
								  vertRoughness,
								  vertReflectiveness),

						   Vertex(vert3Pos, // Lower right (v3)
								  vert3Pos,
						  		  vertColor,
								  vertColor,
						  		  NormalBuilder::BuildNormal(vert3Pos),
								  NormalBuilder::BuildNormal(vert0Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert3Pos),
						  		  vertRoughness,
						  		  vertReflectiveness) };

	// Initialise indices
	// Each set of three values is one triangle
	long indices[ATHRU_RECT_INDEX_COUNT] = { 0,
										     1,
										     2,

										     2,
										     3,
										     1 };


	// Set up the description of the static vertex buffer
	D3D11_BUFFER_DESC vertBufferDesc;
	vertBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertBufferDesc.ByteWidth = sizeof(Vertex) * 4;
	vertBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertBufferDesc.MiscFlags = 0;
	vertBufferDesc.StructureByteStride = 0;

	// Place a reference to the vertices in a subresource structure
	D3D11_SUBRESOURCE_DATA vertData;
	vertData.pSysMem = vertices;
	vertData.SysMemPitch = 0;
	vertData.SysMemSlicePitch = 0;

	// Create the vertex buffer
	result = d3dDevice->CreateBuffer(&vertBufferDesc, &vertData, &vertBuffer);

	// Set up the description of the static index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * ATHRU_RECT_INDEX_COUNT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Place a reference to the vertex indices in a subresource structure
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer
	result = d3dDevice->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Check if anything failed during the geometry setup
	assert(SUCCEEDED(result));
}

AthruRect::~AthruRect()
{
	// Release the render target view
	renderTarget->Release();
}

ID3D11RenderTargetView* AthruRect::GetRenderTarget()
{
	return renderTarget;
}
