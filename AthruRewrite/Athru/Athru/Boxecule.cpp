#include "ServiceCentre.h"
#include "Boxecule.h"

Boxecule::Boxecule(Material boxeculeMaterial) : 
				   Mesh(boxeculeMaterial)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

	// Material associated in the initializer list, so no need to do that here

	// Cache the color associated with [this]
	DirectX::XMFLOAT4 vertColor = material.GetColorData();

	// Store min bounding cube position, max bounding cube position, and the difference (range) between them
	DirectX::XMFLOAT4 minBoundingPos = DirectX::XMFLOAT4(-0.5f, -0.5f, -0.5f, 1);
	DirectX::XMFLOAT4 maxBoundingPos = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1);
	DirectX::XMFLOAT4 boundingRange = DirectX::XMFLOAT4(maxBoundingPos.x - minBoundingPos.x,
														maxBoundingPos.y - minBoundingPos.y, 
														maxBoundingPos.z - minBoundingPos.z, 1);

	// Generate + cache vertex positions
	DirectX::XMFLOAT4 vert0Pos = DirectX::XMFLOAT4(-0.5f, 0.5f, -0.5f, 1); // Front plane, upper left (v0)
	DirectX::XMFLOAT4 vert1Pos = DirectX::XMFLOAT4(0.5f, 0.5f, -0.5f, 1); // Front plane, upper right (v1)
	DirectX::XMFLOAT4 vert2Pos = DirectX::XMFLOAT4(-0.5f, -0.5f, -0.5f, 1); // Front plane, lower left (v2)
	DirectX::XMFLOAT4 vert3Pos = DirectX::XMFLOAT4(0.5f, -0.5f, -0.5f, 1); // Front plane, lower right (v3)
	DirectX::XMFLOAT4 vert4Pos = DirectX::XMFLOAT4(0.5f, -0.5f, 0.5f, 1); // Back plane, lower right (v4)
	DirectX::XMFLOAT4 vert5Pos = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1); // Back plane, upper right (v5)
	DirectX::XMFLOAT4 vert6Pos = DirectX::XMFLOAT4(-0.5f, 0.5f, 0.5f, 1); // Back plane, upper left (v6)
	DirectX::XMFLOAT4 vert7Pos = DirectX::XMFLOAT4(-0.5f, -0.5f, 0.5f, 1); // Back plane, lower left (v7)

	// Initialise vertices
	Vertex vertices[8] = { Vertex(vert0Pos, // Front plane, upper left (v0)
								  vertColor, 
								  NormalBuilder::ForRegular(vert0Pos), 
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert0Pos)),

						   Vertex(vert1Pos, // Front plane, upper right (v1)
								  vertColor, 
								  NormalBuilder::ForRegular(vert1Pos), 
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert1Pos)),

						   Vertex(vert2Pos, // Front plane, lower left (v2)
								  vertColor, 
								  NormalBuilder::ForRegular(vert2Pos), 
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert2Pos)),

						   Vertex(vert3Pos, // Front plane, lower right (v3)
								  vertColor, 
								  NormalBuilder::ForRegular(vert3Pos), 
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert3Pos)),

						   Vertex(vert4Pos, // Back plane, lower right (v4)
								  vertColor, 
								  NormalBuilder::ForRegular(vert4Pos), 
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert4Pos)),

						   Vertex(vert5Pos, // Back plane, upper right (v5)
								  vertColor, 
								  NormalBuilder::ForRegular(vert5Pos), 
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert5Pos)),

						   Vertex(vert6Pos, // Back plane, upper left (v6) 
								  vertColor, 
								  NormalBuilder::ForRegular(vert6Pos), 
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert6Pos)),

						   Vertex(vert6Pos, // Back plane, lower left (v7)
								  vertColor, 
								  NormalBuilder::ForRegular(vert7Pos), 
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert7Pos)) };

	// Initialise indices
	// Each set of three values is one triangle
	long indices[BOXECULE_INDEX_COUNT] = { 0,
										   1,
										   2,
										   
										   2,
										   1,
										   3,
										   
										   2,
										   3,
										   4,
										   
										   4,
										   3,
										   1,
										   
										   4,
										   1,
										   5,
										   
										   5,
										   1,
										   0,
										   
										   5,
										   0,
										   6,
										   
										   6,
										   0,
										   2,
										   
										   6,
										   2,
										   7,
										   
										   2,
										   4,
										   7,
										   
										   7,
										   4,
										   6,
										   
										   4,
										   5,
										   6 };


	// Set up the description of the static vertex buffer
	D3D11_BUFFER_DESC vertBufferDesc;
	vertBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertBufferDesc.ByteWidth = sizeof(Vertex) * 8;
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
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * BOXECULE_INDEX_COUNT;
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

Boxecule::~Boxecule()
{
	// Release the index buffer
	indexBuffer->Release();
	indexBuffer = 0;

	// Release the vertex buffer
	vertBuffer->Release();
	vertBuffer = 0;
}
