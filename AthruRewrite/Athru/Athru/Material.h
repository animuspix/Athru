#pragma once

#include "Sound.h"

class Material
{
	public:
		Material();
		Material(Sound sonicStuff, 
				 float r, float g, float b, float a);
		~Material();
		
	private:
		Sound sonicData;
		float color[4];
};

