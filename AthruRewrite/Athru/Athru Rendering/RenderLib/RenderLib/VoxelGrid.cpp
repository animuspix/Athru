
#include "UtilityServiceCentre.h"
#include "SceneVertex.h"
#include "VoxelGrid.h"

VoxelGrid::VoxelGrid(ID3D11Device* device)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

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
	SceneVertex vertices[8] = { SceneVertex(vert0Pos),    // Front plane, upper left (v0)
								SceneVertex(vert1Pos),    // Front plane, upper right (v1)
								SceneVertex(vert2Pos),    // Front plane, lower left (v2)
								SceneVertex(vert3Pos),    // Front plane, lower right (v3)
								SceneVertex(vert4Pos),    // Back plane, lower right (v4)
								SceneVertex(vert5Pos),    // Back plane, upper right (v5)
								SceneVertex(vert6Pos),    // Back plane, upper left (v6)
								SceneVertex(vert7Pos), }; // Back plane, lower left (v7))

	// Initialise indices
	// Each set of three values is one triangle
	fourByteUnsigned indices[VOXEL_INDEX_COUNT] = { 0,
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
	vertBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertBufferDesc.ByteWidth = sizeof(SceneVertex) * 8;
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
	result = device->CreateBuffer(&vertBufferDesc, &vertData, &vertBuffer);
	assert(SUCCEEDED(result));

	// Set up the description of the static index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(fourByteUnsigned) * VOXEL_INDEX_COUNT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Place a reference to the vertex indices in a subresource structure
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	indexData.SysMemPitch = sizeof(indices);
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
	assert(SUCCEEDED(result));

	// Initialise instances on the heap

	// Allocate memory
	VoxelInstance* instances = new VoxelInstance[GraphicsStuff::VOXEL_GRID_VOLUME];

	// Generate + define 3D texture coordinates for each instance
	float u;
	float v;
	float w;
	eightByteUnsigned linearIndex = -1;
	for (eightByteUnsigned i = 0; i < GraphicsStuff::VOXEL_GRID_WIDTH; i += 1)
	{
		for (eightByteUnsigned j = 0; j < GraphicsStuff::VOXEL_GRID_WIDTH; j += 1)
		{
			for (eightByteUnsigned k = 0; k < GraphicsStuff::VOXEL_GRID_WIDTH; k += 1)
			{
				// Generate u by compressing i into the interval 0...1
				u = (float)i / (GraphicsStuff::VOXEL_GRID_WIDTH - 1);

				// Generate v by compressing j into the interval 0...1
				v = (float)j / (GraphicsStuff::VOXEL_GRID_WIDTH - 1);

				// Generate w by compressing k into the interval 0...1
				w = (float)k / (GraphicsStuff::VOXEL_GRID_WIDTH - 1);

				// Use the linear index to construct the relevant instance
				// with the coordinates generated above
				linearIndex += 1;
				instances[linearIndex] = DirectX::XMFLOAT3(u, v, w);
			}
		}
	}

	// Set up the description of the static instance buffer
	D3D11_BUFFER_DESC instanceBufferDesc;
	instanceBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	instanceBufferDesc.ByteWidth = (fourByteUnsigned)(sizeof(VoxelInstance)) * (fourByteUnsigned)GraphicsStuff::VOXEL_GRID_VOLUME;
	instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instanceBufferDesc.CPUAccessFlags = 0;
	instanceBufferDesc.MiscFlags = 0;
	instanceBufferDesc.StructureByteStride = 0;

	// Place a reference to the vertex indices in a subresource structure
	D3D11_SUBRESOURCE_DATA instanceData;
	instanceData.pSysMem = instances;
	instanceData.SysMemPitch = sizeof(instances);
	instanceData.SysMemSlicePitch = 0;

	// Create the instance buffer
	result = device->CreateBuffer(&instanceBufferDesc, &instanceData, &instanceBuffer);
	assert(SUCCEEDED(result));

	// No reason to keep the instance heap-array around, so release it + send the
	// associated reference to [nullptr]
	delete instances;
	instances = nullptr;
}

VoxelGrid::~VoxelGrid()
{
	// Most destructions handled within [Mesh]

	// Release voxel instance data
	instanceBuffer->Release();
	instanceBuffer = nullptr;
}

void VoxelGrid::PassToGPU(ID3D11DeviceContext * deviceContext)
{
	// Define vertex/instance buffer strides and offsets
	fourByteUnsigned strides[2] = { (fourByteUnsigned)sizeof(SceneVertex),
								    (fourByteUnsigned)sizeof(VoxelInstance) };

	fourByteUnsigned offsets[2] = { 0, 0 };

	// Store the vertex/instance buffers in a single merged array for easy
	// GPU access
	ID3D11Buffer* voxelBuffers[2] = { vertBuffer,
									  instanceBuffer, };

	// Pass the merged vertex/instance buffer into the input assembler
	deviceContext->IASetVertexBuffers(0, 2, voxelBuffers, strides, offsets);

	// Pass the index buffer into the input assembler
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the 2D primitive type used to create each voxel; scene voxels are
	// literally just cubes, so triangle strips make the most sense
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Release the references created when the voxel buffer array was
	// initialized
	//voxelBuffers[0]->Release();
	//voxelBuffers[1]->Release();
}

// Push constructions for this class through Athru's custom allocator
void* VoxelGrid::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<VoxelGrid>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void VoxelGrid::operator delete(void* target)
{
	return;
}