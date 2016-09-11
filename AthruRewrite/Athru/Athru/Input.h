#pragma once

#include "Service.h"

class Input : public Service
{
	public:
		Input();
		~Input();

		void KeyDown(unsigned int);
		void KeyUp(unsigned int);
		bool IsKeyDown(unsigned int);

	private:
		bool keys[256];
};

