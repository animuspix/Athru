#include "MathIncludes.h"
#include "ServiceCentre.h"
#include <cstdlib>
#include "Camera.h"
#include "Boxecule.h"
#include "Chunk.h"

Chunk::Chunk(byteSigned offsetFromHomeX, byteSigned offsetFromHomeZ)
{
	chunkPoints[0] = _mm_set_ps(0, (float)(offsetFromHomeZ), 0, (float)offsetFromHomeX);
	chunkPoints[1] = _mm_set_ps(0, (float)(offsetFromHomeZ - CHUNK_WIDTH), 0, (float)offsetFromHomeX);
	chunkPoints[2] = _mm_set_ps(0, (float)(offsetFromHomeZ), 0, (float)(offsetFromHomeX + CHUNK_WIDTH));
	chunkPoints[3] = _mm_set_ps(0, (float)(offsetFromHomeZ - CHUNK_WIDTH), 0, (float)(offsetFromHomeX + CHUNK_WIDTH));

	subChunks = DEBUG_NEW SubChunk*[SUB_CHUNK_VOLUME];

	for (byteUnsigned i = 0; i < SUB_CHUNKS_PER_CHUNK; i += 1)
	{
		subChunks[i] = new SubChunk(this, i, offsetFromHomeX, offsetFromHomeZ);
	}
}

Chunk::~Chunk()
{
	for (byteUnsigned i = 0; i < SUB_CHUNKS_PER_CHUNK; i += 1)
	{
		subChunks[i]->~SubChunk();
	}

	delete subChunks;
	subChunks = nullptr;
}

void Chunk::Update(DirectX::XMVECTOR& playerPosition)
{
	// Weather updates (weather would really be calculated per-planet,
	// but some processing would still happen in the chunks closest to
	// the player), organism updates, and terrain generation + disk
	// storage (if terrain is never passed to disk then it won't
	// persist between plays) will happen here in the future;
	// sticking to a simple lit + shadowed bumpy plane for now :)

	// Realllllllllly hacky way to keep the demo directional light
	// within the FOV so it can't be culled; this needs to be replaced
	// with a more permanent solution (e.g. nothing moves at all,
	// terrain + lighting is streamed to the player's "position" on the
	// planet) as soon as possible
	Boxecule* directionalLightBoxecule = subChunks[SUB_CHUNKS_PER_CHUNK - 1]->GetStoredBoxecules()[CHUNK_WIDTH / 2];
	subChunks[SUB_CHUNKS_PER_CHUNK - 1]->GetStoredBoxecules()[CHUNK_WIDTH / 2]->FetchTransformations().pos = _mm_add_ps(playerPosition, _mm_set_ps(0, 1, 3, 1));
}

bool Chunk::GetVisibility(Camera* mainCamera)
{
	// Generate a 2D view triangle so we can check chunk visibility
	// without expensive frustum testing (chunks are parallel to ZX
	// anyways, so there's nothing really lost by only checking for
	// intersections within that plane)

	// Point/triangle culling algorithm found here:
	// http://www.nerdparadise.com/math/pointinatriangle

	// Cache camera position + rotation
	DirectX::XMVECTOR cameraPos = mainCamera->GetTranslation();
	DirectX::XMVECTOR cameraRotation = mainCamera->GetRotationQuaternion();

	// Construct the view triangle
	DirectX::XMVECTOR viewTriVertA = cameraPos;
	DirectX::XMVECTOR viewTriVertB = DirectX::XMVector3Rotate(_mm_add_ps(viewTriVertA, _mm_set_ps(0, SCREEN_FAR, 0, tan(HORI_FIELD_OF_VIEW_RADS / 2) * SCREEN_FAR)), cameraRotation);
	DirectX::XMVECTOR viewTriVertC = DirectX::XMVector3Rotate(_mm_add_ps(viewTriVertA, _mm_set_ps(0, SCREEN_FAR, 0, (tan(HORI_FIELD_OF_VIEW_RADS / 2) * SCREEN_FAR) * -1)), cameraRotation);

	// Test chunk points against the view triangle
	float crossProductMagnitudesZX[4][3];
	for (byteUnsigned i = 0; i < 4; i += 1)
	{
		DirectX::XMVECTOR vectorDiffZXA = _mm_sub_ps(chunkPoints[i], viewTriVertA);
		DirectX::XMVECTOR vectorDiffZXB = _mm_sub_ps(viewTriVertB, viewTriVertA);
		DirectX::XMVECTOR vectorDiffZXC = _mm_sub_ps(chunkPoints[i], viewTriVertB);

		DirectX::XMVECTOR vectorDiffZXD = _mm_sub_ps(viewTriVertC, viewTriVertB);
		DirectX::XMVECTOR vectorDiffZXE = _mm_sub_ps(chunkPoints[i], viewTriVertC);
		DirectX::XMVECTOR vectorDiffZXF = _mm_sub_ps(viewTriVertA, viewTriVertC);

		crossProductMagnitudesZX[i][0] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffZXA, vectorDiffZXB)));
		crossProductMagnitudesZX[i][1] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffZXC, vectorDiffZXD)));
		crossProductMagnitudesZX[i][2] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffZXE, vectorDiffZXF)));
	}

	// Return visibility
	// (defined as whether any chunk point is within the view triangle;
	// see the algorithm link above for how that's evaluated)
	return std::signbit(crossProductMagnitudesZX[0][0]) == std::signbit(crossProductMagnitudesZX[1][0]) == std::signbit(crossProductMagnitudesZX[2][0]) == std::signbit(crossProductMagnitudesZX[3][0]) ||
		   std::signbit(crossProductMagnitudesZX[0][1]) == std::signbit(crossProductMagnitudesZX[1][1]) == std::signbit(crossProductMagnitudesZX[2][1]) == std::signbit(crossProductMagnitudesZX[3][1]) ||
		   std::signbit(crossProductMagnitudesZX[0][2]) == std::signbit(crossProductMagnitudesZX[1][2]) == std::signbit(crossProductMagnitudesZX[2][2]) == std::signbit(crossProductMagnitudesZX[3][2]);
}

DirectX::XMVECTOR* Chunk::GetChunkPoints()
{
	return chunkPoints;
}

SubChunk** Chunk::GetSubChunks()
{
	return subChunks;
}

// Push constructions for this class through Athru's custom allocator
void* Chunk::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Chunk>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Chunk::operator delete(void* target)
{
	return;
}