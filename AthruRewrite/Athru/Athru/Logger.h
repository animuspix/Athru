#include <iostream>
#include "Typedefs.h"
#pragma once

template<typename loggableType> class Logger
{
	public:
		Logger(char* loggingTo);
		~Logger();

		// Functions logging data to CMD (single variables)
		void LogTextCharToCMD(char textCharLogging);
		void LogSignedByteToCMD(byteSigned signedByteLogging);
		void LogUnsignedByteToCMD(byteUnsigned unsignedByteLogging);
		void LogSignedDoubleByteToCMD(twoByteSigned signedDoubleByteLogging);
		void LogUnsignedDoubleByteToCMD(twoByteUnsigned UnsignedDoubleByteLogging);
		void LogSignedQuadByteToCMD(fourByteSigned unsignedByteLogging);
		void LogUnsignedQuadByteToCMD(fourByteUnsigned unsignedByteLogging);
		void LogSignedOctaByteToCMD(eightByteSigned unsignedByteLogging);
		void LogUnsignedOctaByteToCMD(eightByteUnsigned unsignedByteLogging);
		void LogAddressToCMD(void* addressLogging);

		// Logging data to CMD (single variables)
		void LogToCMD(loggableType dataLogging)
		{
			// Check if type is printable before passing to std::cout
		}

		// Logging data to file (single variables)
		void LogToFile(loggableType dataLogging)
		{
			
		}

		// Functions logging data to CMD (serial variables, e.g. arrays, strings, etc.)
		void LogStringToCMD(char* stringLogging);
		void LogSignedBytesToCMD(byteSigned* signedByteLogging);
		void LogUnsignedBytesToCMD(byteUnsigned* unsignedByteLogging);
		void LogSignedDoubleBytesToCMD(twoByteSigned* signedDoubleByteLogging);
		void LogUnsignedDoubleBytesToCMD(twoByteUnsigned* UnsignedDoubleByteLogging);
		void LogSignedQuadBytesToCMD(fourByteSigned* unsignedByteLogging);
		void LogUnsignedQuadBytesToCMD(fourByteUnsigned* unsignedByteLogging);
		void LogSignedOctaBytesToCMD(eightByteSigned* unsignedByteLogging);
		void LogUnsignedOctaBytesToCMD(eightByteUnsigned* unsignedByteLogging);

		// Functions logging data to file (serial variables, e.g. arrays and strings)
		void LogStringToFile(char* stringLogging);
		void LogSignedByteToFile(byteSigned signedByteLogging);
		void LogUnsignedByteToFile(byteUnsigned unsignedByteLogging);
		void LogSignedDoubleByteToFile(twoByteSigned signedDoubleByteLogging);
		void LogUnsignedDoubleByteToFile(twoByteUnsigned UnsignedDoubleByteLogging);
		void LogSignedQuadByteToFile(fourByteSigned unsignedByteLogging);
		void LogUnsignedQuadByteToFile(fourByteUnsigned unsignedByteLogging);
		void LogSignedOctaByteToFile(eightByteSigned unsignedByteLogging);
		void LogUnsignedOctaByteToFile(eightByteUnsigned unsignedByteLogging);

	private:
		char* logFilePath;
		// Memoize file data here
};

