#pragma once

#include "Mesh.h"

class VertexBlend
{
	public:
		VertexBlend();
		~VertexBlend();

	private:
		// Base mesh
		Mesh LHS;

		// Target mesh
		Mesh RHS;

		// Blend duration (seconds)
		float duration;

		// Skeletal animation will be used instead of
		// vertex animation in Athru (this exists because
		// it's fast and I need it for schoolwork) so don't
		// include any markers for whether or not the
		// animation loops or what sort of falloff it uses
};