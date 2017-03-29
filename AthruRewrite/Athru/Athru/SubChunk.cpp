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
	// sub-chunk will just contain a plane with occasional towers instead :P
	storedBoxecules = DEBUG_NEW Boxecule*[SUB_CHUNK_VOLUME];

	// Cache a local reference to the texture manager
	TextureManager* textureManagerPttr = ServiceCentre::AccessTextureManager();

	// Build chunks
	for (eightByteSigned i = 0; i < SUB_CHUNK_VOLUME; i += 1)
	{
		// Make boxecules invisible unless [this] contains y-values below [SUB_CHUNKS_PER_CHUNK / 2]
		// OR the iterator is equal to [2]; this produces a flat plane with nice floating boxecules that
		// can be used for lighting demonstrations
		float alpha = (float)(index <= (SUB_CHUNKS_PER_CHUNK / 2) || i == 2);

		// Minor color variance to show individual blocks within the plane
		//float red = 1.0f / (rand() % 10);
		//float green = 0.4f;
		//float blue = 0.4f;

		// Set up boxecule lighting properties
		// Only allow visible boxecules (that are hovering above the main visible plane) to illuminate
		// other boxecules
		// Faintly randomize for some interesting intensity variations
		// Zeroed out for now; strange and mysterious memory overflows occur if anything non-zero is assigned into it :|
		float illumIntensity = (((byteUnsigned)alpha) && (index > SUB_CHUNKS_PER_CHUNK / 2)) * (float)(1 / ((rand() % 10) + 1));

		// Point lights on one side, spot-lights on the other
		AVAILABLE_LIGHTING_SHADERS lightType = (AVAILABLE_LIGHTING_SHADERS)(((parentOffsetX < 0) + (2 * (parentOffsetX >= 0))));

		// Construct illumination data from the values above
		Luminance illumination = Luminance(illumIntensity, lightType);

		// Set up the average sound to play when the boxecule interacts
		// with its environment
		Sound activeTone = Sound(0.5f, 0.5f);

		// Core boxecule construction
		// Sun generation is temporary; I'd much rather make an actual SkyChunk that can handle clouds and things
		// as well
		// But I made the decision to do my long-term commercial project for schoolwork, and now it's two days behind
		// schedule, so just go with the mediocre hacky option for now :P
		// Make a sun instead of an ordinary boxecule if [this] is the uppermost index,
		// the iterator is halfway across the sub-chunk, and [this] is inside the home chunk
		DirectX::XMVECTOR boxeculeRot;
		if ((index == (SUB_CHUNKS_PER_CHUNK - 1)) && (i == CHUNK_WIDTH / 2) && (parentOffsetX == 0 && parentOffsetZ == 0))
		{
			storedBoxecules[i] = new Boxecule(Material(Sound(0.5f, 0.5f),
													   Luminance(0.8f, AVAILABLE_LIGHTING_SHADERS::DIRECTIONAL_LIGHT),
													   1.0f, 1.0f, 1.0f, 1.0f,
													   0.0f,
													   0.0f,
													   AVAILABLE_OBJECT_SHADERS::RASTERIZER,
													   AthruTexture(ServiceCentre::AccessTextureManager()->GetTexture(AVAILABLE_TEXTURES::BLANK_WHITE))));

			// Rotate the pseudo-sun 22.5 degrees about [y]
			boxeculeRot = DirectX::XMQuaternionRotationRollPitchYaw(22.5, 0, 0);
		}

		else
		{
			storedBoxecules[i] = new Boxecule(Material(activeTone,
												       illumination,
												       1.0f, 1.0f, 1.0f, alpha,
													   0.8f,
													   0.2f,
												       AVAILABLE_OBJECT_SHADERS::TEXTURED_RASTERIZER,
												       textureManagerPttr->GetTexture(AVAILABLE_TEXTURES::BLANK_WHITE)));

			// Apply neutral rotation to non-sun boxecules
			boxeculeRot = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, 0);
		}

		// Place the generated boxecule somewhere within the plane (the towers aren't actually separate units, they've just been
		// highlighted by making them the only opaque boxecules above "ground level" ([SUB_CHUNKS_PER_CHUNK] / 2)
		DirectX::XMVECTOR boxeculePos = _mm_set_ps(1, ((float)((i / (CHUNK_WIDTH * SUB_CHUNK_DEPTH)) % (CHUNK_WIDTH))) + parentOffsetZ,
													   (float)(((i / CHUNK_WIDTH) % SUB_CHUNK_DEPTH) + index * SUB_CHUNK_DEPTH),
													   (float)(i % CHUNK_WIDTH) + parentOffsetX);

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