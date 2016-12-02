#include "stdlib.h"
#include "Logger.h"

Logger::Logger(char* loggingTo)
{
	logFilePath = loggingTo;
}

Logger::~Logger()
{
}

void Logger::LogTextCharToCMD(char textCharLogging)
{
	printf("%s", stringLogging);
}

void Logger::LogSignedByteToCMD(byteSigned signedByteLogging)
{
	printf("%s", stringLogging);
}

void Logger::LogUnsignedByteToCMD(byteUnsigned unsignedByteLogging)
{
	printf("%s", stringLogging);
}

void Logger::LogSignedDoubleByteToCMD(twoByteSigned signedDoubleByteLogging)
{
	printf("%s", stringLogging);
}

void Logger::LogUnsignedDoubleByteToCMD(twoByteUnsigned UnsignedDoubleByteLogging)
{
	printf("%s", stringLogging);
}

void Logger::LogSignedQuadByteToCMD(fourByteSigned unsignedByteLogging)
{
	printf("%s", stringLogging);
}

void Logger::LogUnsignedQuadByteToCMD(fourByteUnsigned unsignedByteLogging)
{
	printf("%s", stringLogging);
}

void Logger::LogSignedOctaByteToCMD(eightByteSigned unsignedByteLogging)
{
	printf("%s", stringLogging);
}

void Logger::LogUnsignedOctaByteToCMD(eightByteUnsigned unsignedOctaByteLogging)
{
	printf("%ull", unsignedByteLogging);
}

void Logger::LogAddressToCMD(void* addressLogging)
{
	printf("%p", addressLogging);
}

void Logger::LogStringToFile(char* stringLogging)
{

}

void Logger::LogTextCharToFile(char textCharLogging)
{

}

void Logger::LogSignedByteToFile(byteSigned signedByteLogging)
{

}

void Logger::LogUnsignedByteToFile(byteUnsigned unsignedByteLogging)
{

}

void Logger::LogSignedDoubleByteToFile(twoByteSigned signedDoubleByteLogging)
{

}

void Logger::LogUnsignedDoubleByteToFile(twoByteUnsigned UnsignedDoubleByteLogging)
{

}

void Logger::LogSignedQuadByteToFile(fourByteSigned unsignedByteLogging)
{

}

void Logger::LogUnsignedQuadByteToFile(fourByteUnsigned unsignedByteLogging)
{

}

void Logger::LogSignedOctaByteToFile(eightByteSigned unsignedByteLogging)
{

}

void Logger::LogUnsignedOctaByteToFile(eightByteUnsigned unsignedByteLogging)
{

}

void Logger::LogStringToCMD(char* stringLogging)
{
	printf("%s", stringLogging);
}