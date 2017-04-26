#include "TempThirdParty\WaveFrontReader.h"
#include "ServiceCentre.h"
#include "DemoMeshImport.h"

// Import process supported by the Wavefront Reader
// utility included with the DirectXMesh library; thanks! :D
// DirectXMesh is hosted over on GitHub:
// https://github.com/Microsoft/DirectXMesh

DemoMeshImport::DemoMeshImport(ID3D11Device* d3dDevice)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

	// Assign a default material
	material = Material();

	// Assign a custom texture ("custom" in the sense I generated it a few weeks ago
	// and thought recycling it here could be fun :P)
	material.SetTexture(ServiceCentre::AccessTextureManager()->GetExternalTexture2D(AVAILABLE_EXTERNAL_TEXTURES::IMPORTED_MESH_TEXTURE));

	// Cache the color associated with [this]
	DirectX::XMFLOAT4 vertColor = material.GetColorData();
	vertColor.w = 1.0f;

	// Cache the roughness and reflectiveness associated with [this]
	float vertRoughness = material.GetRoughness();
	float vertReflectiveness = material.GetReflectiveness();

	// Load data from the base demo-mesh
	WaveFrontReader<fourByteUnsigned> baseMeshReader;
	baseMeshReader.Load(L"demoModelImportBase.obj");

	// Extract vertex count + vertex data from the base demo-mesh
	fourByteUnsigned vertCount = (fourByteUnsigned)baseMeshReader.vertices.size();
	std::vector<WaveFrontReader<fourByteUnsigned>::Vertex> baseMeshVerts = baseMeshReader.vertices;

	// Extract bounding square dimensions (useful for planar UV mapping)
	DirectX::XMFLOAT4 maxPos = DirectX::XMFLOAT4(baseMeshVerts[0].position.x, baseMeshVerts[0].position.y, baseMeshVerts[0].position.z, 1);
	DirectX::XMFLOAT4 minPos = DirectX::XMFLOAT4(baseMeshVerts[0].position.x, baseMeshVerts[0].position.y, baseMeshVerts[0].position.z, 1);
	DirectX::XMFLOAT4 posRange = DirectX::XMFLOAT4(0, 0, 0, 0);
	for (fourByteUnsigned i = 0; i < vertCount; i += 1)
	{
		DirectX::XMFLOAT4 currVertPos = DirectX::XMFLOAT4(baseMeshVerts[i].position.x, baseMeshVerts[i].position.y, baseMeshVerts[i].position.z, 1);
		if (MathsStuff::sisdVectorMagnitude(currVertPos) > MathsStuff::sisdVectorMagnitude(maxPos))
		{
			maxPos = currVertPos;
		}

		else if (MathsStuff::sisdVectorMagnitude(currVertPos) < MathsStuff::sisdVectorMagnitude(minPos))
		{
			minPos = currVertPos;
		}
	}

	DirectX::XMVECTOR ssePosRange = _mm_sub_ps(DirectX::XMLoadFloat4(&maxPos), DirectX::XMLoadFloat4(&minPos));
	DirectX::XMStoreFloat4(&posRange, ssePosRange);

	// Load data from the animation-target mesh
	WaveFrontReader<fourByteUnsigned> targetMeshReader;
	targetMeshReader.Load(L"demoModelImportTarget.obj");

	// Extract vertex data from the base demo-mesh
	std::vector<WaveFrontReader<fourByteUnsigned>::Vertex> targetMeshVerts = targetMeshReader.vertices;

	// Initialise vertices
	Vertex* vertices = new Vertex[vertCount];
	for (fourByteUnsigned i = 0; i < vertCount; i += 1)
	{
		DirectX::XMFLOAT4 currBaseVertPos = DirectX::XMFLOAT4(baseMeshVerts[i].position.x, baseMeshVerts[i].position.y, baseMeshVerts[i].position.z, 1);
		DirectX::XMFLOAT4 currTargetVertPos = DirectX::XMFLOAT4(targetMeshVerts[i].position.x, targetMeshVerts[i].position.y, targetMeshVerts[i].position.z, 1);
		DirectX::XMFLOAT4 currBaseVertNormal = DirectX::XMFLOAT4(baseMeshVerts[i].normal.x, baseMeshVerts[i].normal.y, baseMeshVerts[i].normal.z, 1);
		DirectX::XMFLOAT4 currTargetVertNormal = DirectX::XMFLOAT4(targetMeshVerts[i].normal.x, targetMeshVerts[i].normal.y, targetMeshVerts[i].normal.z, 1);

		vertices[i] = Vertex(currBaseVertPos,
							 currTargetVertPos,
							 vertColor,
							 DirectX::XMFLOAT4(0.1f, 0.2f, 0.3f, 0.1f),
							 currBaseVertNormal,
							 currTargetVertNormal,
							 PlanarUnwrapper::Unwrap(minPos, maxPos, posRange, currBaseVertPos),
							 vertRoughness,
							 vertReflectiveness);
	}

	// Initialise indices
	std::vector<fourByteUnsigned> baseMeshIndices = baseMeshReader.indices;
	fourByteUnsigned* indices = new fourByteUnsigned[(fourByteUnsigned)baseMeshReader.indices.size()];
	indexCount = (fourByteUnsigned)baseMeshIndices.size();
	__analysis_assume(indexCount == 0);
	for (fourByteUnsigned i = 0; i < indexCount; i += 1)
	{
		indices[i] = baseMeshIndices[i];
	}

	// Set up the description of the static vertex buffer
	D3D11_BUFFER_DESC vertBufferDesc;
	vertBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertBufferDesc.ByteWidth = sizeof(Vertex) * vertCount;
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
	indexBufferDesc.ByteWidth = sizeof(fourByteUnsigned) * indexCount;
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

	// Release temporary vertex array
	delete[] vertices;
	vertices = nullptr;

	// Release temporary index array
	delete[] indices;
	indices = nullptr;

	// Initialise the scale, rotation, and position of [this]
	transformations = SQT();
}

DemoMeshImport::~DemoMeshImport()
{
}

void DemoMeshImport::PassToGPU(ID3D11DeviceContext* deviceContext)
{
	// Set vertex buffer stride and offset.
	unsigned int stride = sizeof(Vertex);
	unsigned int offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered
	deviceContext->IASetVertexBuffers(0, 1, &vertBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the 2D primitive type used to create the Mesh; we're making simple
	// geometry, so triangular primitives make the most sense
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

fourByteUnsigned DemoMeshImport::GetIndexCount()
{
	return indexCount;
}
