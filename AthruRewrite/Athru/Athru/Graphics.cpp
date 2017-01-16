#include "ServiceCentre.h"
#include "Graphics.h"

Graphics::Graphics()
{

}

Graphics::~Graphics()
{
}

bool Graphics::Frame()
{
	return true;
}

bool Graphics::Render()
{

	return true;
}

// Push constructions for this class through Athru's custom allocator
void* Graphics::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::Instance().AccessMemory();
	return allocator->AlignedAlloc((fourByteUnsigned)size, 4, false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Graphics::operator delete(void* target)
{
	return;
}