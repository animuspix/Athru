#pragma once

#include <sstream>

class ConsolePrinter
{
	public:
		static void OutputText(std::ostringstream* cStringStream);

	private:
		static char outputString[8191];
};