#pragma once

#include "Typedefs.h"
#include "RenderManager.h"
#include "AthruTexture.h"
#include "Sound.h"

class Material
{
	public:
		Material();
		Material(Sound sonicStuff,
				 float r, float g, float b, float a,
				 DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader0, DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader1,
				 DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader2, DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader3,
				 DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader4, FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader0, 
				 FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader1, FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader2, 
				 FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader3, FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader4, 
				 AthruTexture baseTexture);
		~Material();

		// Retrieve the sound associated with [this]
		Sound& GetSonicData();

		// Retrieve the base color of [this]
		float* GetColorData();

		// Retrieve the raw texture associated with [this]
		AthruTexture& GetTexture();

		// Retrieve the raw texture associated with [this]
		void SetTexture(AthruTexture suppliedTexture);

		// Retrieve the array of deferred rendering shaders associated with
		// [this]
		DEFERRED::AVAILABLE_OBJECT_SHADERS* GetDeferredShaderSet();

		// Retrieve the array of forward rendering shaders associated with
		// [this]
		FORWARD::AVAILABLE_OBJECT_SHADERS* GetForwardShaderSet();

	private:
		Sound sonicData;
		float color[4];
		AthruTexture texture;
		DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShaderSet[5];
		FORWARD::AVAILABLE_OBJECT_SHADERS forwardShaderSet[5];
};

