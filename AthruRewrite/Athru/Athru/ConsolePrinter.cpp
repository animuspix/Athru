#include <windows.h>
#include "Typedefs.h"
#include "ConsolePrinter.h"

char ConsolePrinter::outputString[8191];
void ConsolePrinter::OutputText(std::ostringstream* cStringStream)
{
	strcpy_s(outputString, cStringStream->str().c_str());
	OutputDebugStringA(outputString);
	cStringStream->str("");
}