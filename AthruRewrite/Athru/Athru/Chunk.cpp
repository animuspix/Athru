#include "Chunk.h"

Chunk::Chunk()
{
}

Chunk::~Chunk()
{
}

void Chunk::Update(DirectX::XMVECTOR playerPosition)
{
	// Weather updates, organism updates, and terrain generation
	// + disk storage (if terrain is never passed to disk then it
	// won't persist between plays) will happen here in the future;
	// sticking to a simple lit + shadowed bumpy plane for now :)
}

Boxecule** Chunk::GetChunkBoxecules()
{
	return nullptr;
}
