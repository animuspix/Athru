#include "NormalBuilder.h"
#include "PlanarUnwrapper.h"
#include "UtilityServiceCentre.h"
#include "Boxecule.h"

Boxecule::Boxecule(ID3D11Device* device)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

	// Cache the color associated with [this]
	DirectX::XMFLOAT4 vertColor = DirectX::XMFLOAT4(1, 1, 1, 1);

	// Cache the roughness and reflectiveness associated with [this]
	float vertRoughness = 0.8f;
	float vertReflectiveness = 0.8f;

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
	SceneVertex vertices[8] = { SceneVertex(vert0Pos, // Front plane, upper left (v0)
								  vert0Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert0Pos),
								  NormalBuilder::BuildNormal(vert0Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert0Pos),
								  vertRoughness,
								  vertReflectiveness),

						        SceneVertex(vert1Pos, // Front plane, upper right (v1)
								  vert1Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert1Pos),
								  NormalBuilder::BuildNormal(vert1Pos),
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert1Pos),
								  vertRoughness,
								  vertReflectiveness),

								SceneVertex(vert2Pos, // Front plane, lower left (v2)
								  vert2Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert2Pos),
								  NormalBuilder::BuildNormal(vert2Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert2Pos),
								  vertRoughness,
								  vertReflectiveness),

								SceneVertex(vert3Pos, // Front plane, lower right (v3)
								  vert3Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert3Pos),
								  NormalBuilder::BuildNormal(vert3Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert3Pos),
								  vertRoughness,
								  vertReflectiveness),

								SceneVertex(vert4Pos, // Back plane, lower right (v4)
								  vert4Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert4Pos),
								  NormalBuilder::BuildNormal(vert4Pos),
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert4Pos),
								  vertRoughness,
								  vertReflectiveness),

								SceneVertex(vert5Pos, // Back plane, upper right (v5)
								  vert5Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert5Pos),
								  NormalBuilder::BuildNormal(vert5Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert5Pos),
							      vertRoughness,
								  vertReflectiveness),

								SceneVertex(vert6Pos, // Back plane, upper left (v6)
								  vert6Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert6Pos),
								  NormalBuilder::BuildNormal(vert6Pos),
								  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert6Pos),
								  vertRoughness,
								  vertReflectiveness),

								SceneVertex(vert7Pos, // Back plane, lower left (v7)
								  vert7Pos,
								  vertColor,
								  vertColor,
								  NormalBuilder::BuildNormal(vert7Pos),
								  NormalBuilder::BuildNormal(vert7Pos),
							      PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert7Pos),
								  vertRoughness,
								  vertReflectiveness) };

	// Initialise indices
	// Each set of three values is one triangle
	long indices[BOXECULE_INDEX_COUNT] = { 0,
										   1,
										   2,

										   2,
										   3,
										   1,

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
	vertBufferDesc.ByteWidth = sizeof(SceneVertex) * 8;
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
}

Boxecule::~Boxecule()
{
	// Destructions handled within [Mesh]
}