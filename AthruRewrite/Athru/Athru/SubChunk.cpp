#include "ServiceCentre.h"
#include "Camera.h"
#include "Chunk.h"
#include "SubChunk.h"

SubChunk::SubChunk(Chunk* parent,
				   float index, float parentOffsetX, float parentOffsetZ) :
				   superChunk(parent),
				   subChunkPoints{ _mm_set_ps(1, parentOffsetZ + CHUNK_WIDTH, index * SUB_CHUNK_DEPTH, parentOffsetX + (CHUNK_WIDTH / 2)),
								   _mm_set_ps(1, parentOffsetZ - CHUNK_WIDTH, index * SUB_CHUNK_DEPTH, parentOffsetX + (CHUNK_WIDTH / 2)),
								   _mm_set_ps(1, parentOffsetZ + CHUNK_WIDTH, index * SUB_CHUNK_DEPTH, parentOffsetX + (CHUNK_WIDTH / 2)),
								   _mm_set_ps(1, parentOffsetZ - CHUNK_WIDTH, index * SUB_CHUNK_DEPTH, parentOffsetX + (CHUNK_WIDTH / 2)) }
{
	// Calls to procedural-generation subsystems would happen here,
	// but I should be focussing on rendering stuffs for now; the
	// chunk will just contain a plane instead :P
	storedBoxecules = DEBUG_NEW Boxecule*[SUB_CHUNK_VOLUME];

	for (eightByteSigned i = 0; i < SUB_CHUNK_VOLUME; i += 1)
	{
		float alpha = (float)(index <= (SUB_CHUNKS_PER_CHUNK / 2));
		float red = 1.0f / (rand() % 10);
		storedBoxecules[i] = new Boxecule(Material(Sound(),
												   red, 0.6f, 0.4f, alpha,
												   DEFERRED::AVAILABLE_OBJECT_SHADERS::RASTERIZER,
												   DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
												   AthruTexture()));

		DirectX::XMVECTOR boxeculePos = _mm_set_ps(1, ((float)((i / (CHUNK_WIDTH * SUB_CHUNK_DEPTH)) % (CHUNK_WIDTH))) + parentOffsetZ,
												       (float)(((i / CHUNK_WIDTH) % SUB_CHUNK_DEPTH) + index * SUB_CHUNK_DEPTH),
													   (float)(i % CHUNK_WIDTH) + parentOffsetX);

		DirectX::XMVECTOR boxeculeRot = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, 0);
		storedBoxecules[i]->FetchTransformations() = SQT(boxeculeRot, boxeculePos, 1);
 	}
}

SubChunk::~SubChunk()
{
	for (eightByteUnsigned i = 0; i < SUB_CHUNK_VOLUME; i += 1)
	{
		storedBoxecules[i]->~Boxecule();
	}

	delete storedBoxecules;
	storedBoxecules = nullptr;
}

bool SubChunk::GetVisibility(Camera* mainCamera)
{
	// Generate a 2D view triangle so we can check sub-chunk visibility
	// without expensive frustum testing (sub-chunks are parallel to ZY
	// anyways, so there's nothing really lost by only checking for
	// intersections within that plane)

	// Point/triangle culling algorithm found here:
	// http://www.nerdparadise.com/math/pointinatriangle

	// Cache camera position + rotation
	DirectX::XMVECTOR cameraPos = mainCamera->GetTranslation();
	DirectX::XMVECTOR cameraRotation = mainCamera->GetRotationQuaternion();

	// Construct the view triangle
	DirectX::XMVECTOR viewTriVertA = cameraPos;
	DirectX::XMVECTOR viewTriVertB = DirectX::XMVector3Rotate(_mm_add_ps(viewTriVertA, _mm_set_ps(0, SCREEN_FAR, tan(VERT_FIELD_OF_VIEW_RADS / 2) * SCREEN_FAR, 0)), cameraRotation);
	DirectX::XMVECTOR viewTriVertC = DirectX::XMVector3Rotate(_mm_add_ps(viewTriVertA, _mm_set_ps(0, SCREEN_FAR, (tan(VERT_FIELD_OF_VIEW_RADS / 2) * SCREEN_FAR) * -1, 0)), cameraRotation);

	// Test sub-chunk points against the view triangle
	float crossProductMagnitudes[4][3];
	for (byteUnsigned i = 0; i < 4; i += 1)
	{
		DirectX::XMVECTOR vectorDiffA = _mm_sub_ps(subChunkPoints[i], viewTriVertA);
		DirectX::XMVECTOR vectorDiffB = _mm_sub_ps(viewTriVertB, viewTriVertA);
		DirectX::XMVECTOR vectorDiffC = _mm_sub_ps(subChunkPoints[i], viewTriVertB);

		DirectX::XMVECTOR vectorDiffD = _mm_sub_ps(viewTriVertC, viewTriVertB);
		DirectX::XMVECTOR vectorDiffE = _mm_sub_ps(subChunkPoints[i], viewTriVertC);
		DirectX::XMVECTOR vectorDiffF = _mm_sub_ps(viewTriVertA, viewTriVertC);

		crossProductMagnitudes[i][0] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffA, vectorDiffB)));
		crossProductMagnitudes[i][1] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffC, vectorDiffD)));
		crossProductMagnitudes[i][2] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffE, vectorDiffF)));
	}

	// Return visibility
	// (defined as whether any sub-chunk point is within the view triangle;
	// see the algorithm link above for how that's evaluated)
	return std::signbit(crossProductMagnitudes[0][0]) == std::signbit(crossProductMagnitudes[1][0]) == std::signbit(crossProductMagnitudes[2][0]) == std::signbit(crossProductMagnitudes[3][0]) ||
		   std::signbit(crossProductMagnitudes[0][1]) == std::signbit(crossProductMagnitudes[1][1]) == std::signbit(crossProductMagnitudes[2][1]) == std::signbit(crossProductMagnitudes[3][1]) ||
		   std::signbit(crossProductMagnitudes[0][2]) == std::signbit(crossProductMagnitudes[1][2]) == std::signbit(crossProductMagnitudes[2][2]) == std::signbit(crossProductMagnitudes[3][2]);
}

Boxecule** SubChunk::GetStoredBoxecules()
{
	return storedBoxecules;
}

// Push constructions for this class through Athru's custom allocator
void* SubChunk::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<SubChunk>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void SubChunk::operator delete(void* target)
{
	return;
}