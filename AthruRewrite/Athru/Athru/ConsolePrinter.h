#include <sstream>

#pragma once

class ConsolePrinter
{
	public:
		static void OutputText(std::ostringstream* cStringStream);

	private:
		static char outputString[1023];
};