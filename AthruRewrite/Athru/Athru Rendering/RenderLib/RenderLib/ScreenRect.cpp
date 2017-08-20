#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
#include "PostVertex.h"
#include "ScreenRect.h"

ScreenRect::ScreenRect(ID3D11Device* d3dDevice)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

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
	PostVertex vertices[4] = { PostVertex(vert0Pos, // Upper left (v0)
										  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert0Pos)),

							   PostVertex(vert1Pos, // Upper right (v1)
										  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert1Pos)),

							   PostVertex(vert2Pos, // Lower left (v2)
										  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert2Pos)),

							   PostVertex(vert3Pos, // Lower right (v3)
										  PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert3Pos)) };

	// Initialise indices
	// Each set of three values is one triangle
	fourByteUnsigned indices[GraphicsStuff::SCREEN_RECT_INDEX_COUNT] = { 0,
										        		  1,
										        		  2,

										        		  2,
										        		  3,
										        		  1 };


	// Set up the description of the static vertex buffer
	D3D11_BUFFER_DESC vertBufferDesc;
	vertBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertBufferDesc.ByteWidth = sizeof(PostVertex) * 4;
	vertBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertBufferDesc.CPUAccessFlags = 0;
	vertBufferDesc.MiscFlags = 0;
	vertBufferDesc.StructureByteStride = 0;

	// Place a reference to the vertices in a subresource structure
	D3D11_SUBRESOURCE_DATA vertData;
	vertData.pSysMem = vertices;
	vertData.SysMemPitch = 0;
	vertData.SysMemSlicePitch = 0;

	// Create the vertex buffer
	result = d3dDevice->CreateBuffer(&vertBufferDesc, &vertData, &vertBuffer);
	assert(SUCCEEDED(result));

	// Set up the description of the static index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(fourByteUnsigned) * GraphicsStuff::SCREEN_RECT_INDEX_COUNT;
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
	assert(SUCCEEDED(result));
}

ScreenRect::~ScreenRect()
{
}

void ScreenRect::PassToGPU(ID3D11DeviceContext* context)
{
	// Set vertex buffer stride and offset.
	unsigned int stride = sizeof(PostVertex);
	unsigned int offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered
	context->IASetVertexBuffers(0, 1, &vertBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the 2D primitive type used to create the Mesh; we're making simple
	// geometry, so triangular primitives make the most sense
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}
