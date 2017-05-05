#include "HiLevelServiceCentre.h"
#include "Camera.h"
#include "Critter.h"
#include "Chunk.h"
#include "SubChunk.h"

SubChunk::SubChunk(Chunk* parent,
				   float index, float parentOffsetX, float parentOffsetZ,
				   Critter* testCritter) :
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
	TextureManager* textureManagerPttr = HiLevelServiceCentre::AccessTextureManager();

	// Build chunks
	for (eightByteSigned i = 0; i < SUB_CHUNK_VOLUME; i += 1)
	{
		// Make boxecules invisible unless [this] contains y-values below [SUB_CHUNKS_PER_CHUNK / 2]
		// OR the iterator is equal to [2]; this produces a flat plane with nice floating boxecules that
		// can be used for lighting demonstrations
		float alpha = (float)(index <= (SUB_CHUNKS_PER_CHUNK / 2) || i == 2);

		// Minor color variance to show individual blocks within the plane
		float red = 1.0f / (float)((rand() % 10) + 1);
		float green = 0.4f;
		float blue = 0.4f;

		// Set up boxecule lighting properties
		// Only allow visible boxecules (that are hovering above the main visible plane) to illuminate
		// other boxecules
		// Faintly randomize for some interesting intensity variations
		float illumIntensity = (((byteUnsigned)alpha) && (index > SUB_CHUNKS_PER_CHUNK / 2)) * ((float)(1 / ((rand() % 10) + 1)) * 5);

		// Core boxecule construction
		// Sun generation is temporary; I'd much rather make an actual SkyChunk that can handle clouds and things
		// as well
		// But I made the decision to do my long-term commercial project for schoolwork, and now it's four-ish days behind
		// schedule, so just go with the mediocre hacky option for now :P
		// Make a sun instead of an ordinary boxecule if [this] is the uppermost index,
		// the iterator is halfway across the sub-chunk, and [this] is inside the home chunk
		DirectX::XMVECTOR boxeculeRot = _mm_set_ps(0, 0, 0, 0);
		if ((index == (SUB_CHUNKS_PER_CHUNK - 1)) && (i == CHUNK_WIDTH / 2) && (parentOffsetX == 0 && parentOffsetZ == 0))
		{
			storedBoxecules[i] = new Boxecule();

			// Rotate the pseudo-sun 22.5 degrees about [y]
			boxeculeRot = DirectX::XMQuaternionRotationRollPitchYaw(MathsStuff::PI, 0, 0);
		}

		else if (testCritter != nullptr && i <= (testCritter->GetTorsoTransformations().pos.m128_f32[1]))
		{
			storedBoxecules[i] = new Boxecule(testCritter->GetCritterMaterial());

			// Copy rotation out of [torsoTransformations]
			boxeculeRot = testCritter->GetTorsoTransformations().rotationQuaternion;
		}

		else
		{
			storedBoxecules[i] = new Boxecule(Material(activeTone,
												       illumination,
												       red, green, blue, alpha,
													   (1.0f / (float)(rand() % 10) + 1) + 0.2f, // Randomize roughness to somewhere between 0.2f and 1.0f
													   red, // Tie reflectiveness to the red channel
												       AVAILABLE_OBJECT_SHADERS::TEXTURED_RASTERIZER,
												       textureManagerPttr->GetExternalTexture2D(AVAILABLE_EXTERNAL_TEXTURES::BLANK_WHITE)));

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

Boxecule** SubChunk::GetStoredBoxecules()
{
	return storedBoxecules;
}

// Push constructions for this class through Athru's custom allocator
void* SubChunk::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<SubChunk>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void SubChunk::operator delete(void* target)
{
	return;
}