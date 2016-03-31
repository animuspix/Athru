#pragma once

#include "lightSource.h"

class FocussedLightSource : LightSource {
	public:
		// Method for emitting a [pulse()] - a short,
		// high-energy burst of light; see the comments
		// under [lightSource] for why [pulse] is
		// declared here instead of there
		void pulse();

		FocussedLightSource();
		FocussedLightSource();

	private:

		// Width of the light column to project; the max
		// scale is 32768 if you keep it signed, but
		// 65535 if it's unsigned
		short columnWidth;
};