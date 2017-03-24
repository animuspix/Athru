#include "ServiceCentre.h"
#include "AthruRect.h"

AthruRect::AthruRect(Material rectMaterial, 
					 float baseWidth, float baseHeight):
					 Mesh(rectMaterial)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

	// Material associated in the initializer list, so no need to do that here

	// Cache the color associated with [this]
	DirectX::XMFLOAT4 color = material.GetColorData();

	// Store min bounding cube position, max bounding cube position, and the difference (range) between them
	DirectX::XMFLOAT4 minBoundingPos = DirectX::XMFLOAT4((baseWidth / 2) * -1, (baseHeight / 2) * -1, 0.0f, 1);
	DirectX::XMFLOAT4 maxBoundingPos = DirectX::XMFLOAT4(baseWidth / 2, baseHeight / 2, 0.0f, 1);
	DirectX::XMFLOAT4 boundingRange = DirectX::XMFLOAT4(maxBoundingPos.x - minBoundingPos.x,
														maxBoundingPos.y - minBoundingPos.y,
														maxBoundingPos.z - minBoundingPos.z, 1);

	// Generate + cache vertex positions
	DirectX::XMFLOAT4 vert0Pos = DirectX::XMFLOAT4((baseWidth / 2) * -1, baseHeight / 2, 0.0f, 1); // Upper left (v0)
	DirectX::XMFLOAT4 vert1Pos = DirectX::XMFLOAT4(baseWidth / 2, baseHeight / 2, 0.0f, 1); // Upper right (v1)
	DirectX::XMFLOAT4 vert2Pos = DirectX::XMFLOAT4((baseWidth / 2) * -1, (baseHeight / 2) * -1, 0.0f, 1); // Lower left (v2)
	DirectX::XMFLOAT4 vert3Pos = DirectX::XMFLOAT4(baseWidth / 2, (baseHeight / 2) * -1, 0.0f, 1); // Lower right (v3)

	// Initialise vertices
	Vertex vertices[4] = { Vertex(vert0Pos,
								  color,
								  NormalBuilder::ForRegular(vert0Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert0Pos)), // Upper left (v0)

						   Vertex(vert0Pos,
							      color,
							      NormalBuilder::ForRegular(vert1Pos),
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert1Pos)), // Upper right (v1)

						   Vertex(vert0Pos,
							      color,
							      NormalBuilder::ForRegular(vert2Pos),
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert2Pos)), // Lower left (v2)

						   Vertex(vert0Pos,
								  color,
								  NormalBuilder::ForRegular(vert3Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert3Pos)) }; // Lower right (v3)

	// Initialise indices
	// Each set of three values is one triangle
	long indices[RECT_INDEX_COUNT] = { 0,
									   1,
									   2,
									   
									   2,
									   1,
									   3 };


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
	ID3D11Device* device = ServiceCentre::AccessGraphics()->GetD3D()->GetDevice();
	result = device->CreateBuffer(&vertBufferDesc, &vertData, &vertBuffer);

	// Set up the description of the static index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * RECT_INDEX_COUNT;
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
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Check if anything failed during the geometry setup
	assert(SUCCEEDED(result));

	// Initialise the scale, rotation, and position of [this]
	transformations = SQT();
}

AthruRect::~AthruRect()
{
}