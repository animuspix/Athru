#pragma once

class Sound
{
	public:
		Sound();
		~Sound();

	private:
		// Wave frequency (hertz)
		float freq;

		// Wave amplitude (decibels)
		float vol;
};

