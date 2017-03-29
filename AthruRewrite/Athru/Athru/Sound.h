#pragma once

class Sound
{
	public:
		Sound(float baseFreq, float baseVol);
		~Sound();

	private:
		// Wave frequency (hertz)
		float freq;

		// Wave amplitude (decibels)
		float vol;
};

