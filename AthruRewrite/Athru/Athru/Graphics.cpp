#include "ServiceCentre.h"
#include "Graphics.h"

Graphics::Graphics() : Service("Graphics")
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
