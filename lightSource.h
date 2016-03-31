#pragma once

class LightSource {
	public:
		void fadeIn();
		void fadeOut();

		void switchOn();
		void switchOff();

		void setWaveLength(short newWaveLength);
		void setIntensity(short newIntensity);

		LightSource();
		~LightSource();

	private:
		//long long castRadius;

		// Specifying radius is redundant;
		// in a semi-realistic lighting
		// engine, light propagates until
		// all it's component photons are
		// absorbed regardless of any sort
		// of artificial barrier, and the
		// time it takes for those photons
		// to get caught is directly
		// affected by the emittance
		// intensity of the radiation, so
		// the effect of castRadius can be
		// imitated by just varying the
		// [intensity] of a given
		// [lightSource]

		// Put focussed light sources
		// (e.g. lasers) in a separate
		// class, along with 
		// focussed-only properties
		// such as [heat], [columnWidth],
		// and the [pulse()] method
		// (pulses are possible with 
		// undirected light, but their
		// indirection and comparative
		// weakness compared to directed
		// pulses means that they aren't
		// versatile enough for engraving,
		// communication, or anything else
		// that requires precise, short,
		// high-energy blasts of light)

		// So, intensity has a mile-long
		// list of possible mathematical
		// definitions; it's probably
		// easiest to fudge it with a
		// generic numerical value and
		// hope no-one notices

		// I don't know which is best,
		// so I've set it to [short]
		// for speed + convenience
		
		// Exactly what it says on the
		// tin, the wavelength of the
		// light emitted in nanometres;
		// Wikipedia has a very good
		// chart showing which colours
		// correspond to which 
		// wavelength
		short nanometreWavelength;
		
		// Faux intensity value, ranging
		// from 0 to 65535
		unsigned short intensity;

		// Radiative heat of the light
		// source
		unsigned short heat;

		// Whether or not the light is
		// "on" (more scientifically,
		// whether or not radiation is
		// being emitted)
		bool isActive;
};