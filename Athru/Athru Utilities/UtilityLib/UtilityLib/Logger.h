#pragma once

#include <sstream>
#include <fstream>
#include <tuple>
#include "Typedefs.h"
#include "leakChecker.h"
#include <windows.h>

class Logger
{
	public:
		Logger(char* loggingTo)
		{
			logFilePath = loggingTo;

			// Streams ONLY work if they're allocated on an external heap;
			// ask SO what the fruit is going on there
			logFileStreamPttr = new std::fstream();
			logFileStreamPttr->open(logFilePath, std::ios::out);
			*logFileStreamPttr << "Note:" << '\n';
			*logFileStreamPttr << "Unions + structs/classes are too complicated to easily log members," << '\n';
			*logFileStreamPttr << "and it's impossible to easily log the names of enum values without" << '\n';
			*logFileStreamPttr << "lots of boilerplate code. This has resulted in the following:" << '\n' << '\n';;
			*logFileStreamPttr << "- Athru will only ever log the address + type-id of unions and" << '\n';
			*logFileStreamPttr << "  avoid describing their members" << '\n' << '\n';
			*logFileStreamPttr << "- Athru will only ever log the address + type-id of structs/classes" << '\n';
			*logFileStreamPttr << "  and avoid describing their members" << '\n' << '\n';
			*logFileStreamPttr << "- Athru will only ever log the address, type-ID, and numeric value of enumerations;" << '\n';
			*logFileStreamPttr << "  enum names must be stringified before being passed to the logger" << '\n' << '\n';
			*logFileStreamPttr << "Thank you for reading :)" << '\n' << '\n';
			logFileStreamPttr->close();

			printStreamPttr = new std::ostringstream();
			*printStreamPttr << "Note:" << '\n';
			*printStreamPttr << "Unions + structs/classes are too complicated to easily log members," << '\n';
			*printStreamPttr << "and it's impossible to easily log the names of enum values without" << '\n';
			*printStreamPttr << "lots of boilerplate code. This has resulted in the following:" << '\n' << '\n';
			*printStreamPttr << "- Athru will only ever log the address + type-id of unions and" << '\n';
			*printStreamPttr << "  avoid describing their members" << '\n' << '\n';
			*printStreamPttr << "- Athru will only ever log the address + type-id of structs/classes" << '\n';
			*printStreamPttr << "  and avoid describing their members" << '\n' << '\n';
			*printStreamPttr << "- Athru will only ever log the address, type-ID, and numeric value of enumerations;" << '\n';
			*printStreamPttr << "  enum names must be stringified before being passed to the logger" << '\n' << '\n';
			*printStreamPttr << "Thank you for reading :)" << '\n' << '\n';
			ConsolePrinter::OutputText(printStreamPttr);
		}

		~Logger()
		{
			// Consider refactoring to use smart pointers
			delete logFileStreamPttr;
			logFileStreamPttr = nullptr;

			delete printStreamPttr;
			printStreamPttr = nullptr;
		}

		enum class DESTINATIONS
		{
			CONSOLE,
			LOG_FILE
		};

		// Logging for pure types, or decoded references to types (i.e. any case where [dataLogging]
		// is *not* a pointer to data of type [loggableType])
		template<DESTINATIONS dest = DESTINATIONS::CONSOLE, typename loggableType>
		void Log(loggableType dataLogging, const char* label = "(unlabelled)")
		{
			// Boilerplate to fend off over-eager MSVC [if constexpr] validations
			constexpr bool arithData = std::is_arithmetic<loggableType>{};
			constexpr bool unionData = std::is_union<loggableType>{};
			constexpr bool classStructData = std::is_class<loggableType>{};
			constexpr bool funcData = std::is_function<loggableType>{};
			constexpr bool memFuncPttrData = std::is_member_function_pointer<loggableType>{};
			constexpr bool enumData = std::is_enum<loggableType>{};
			constexpr bool isArith =  arithData &&
									 !unionData &&
									 !classStructData &&
									 !funcData &&
									 !memFuncPttrData &&
									 !enumData;

			constexpr bool isUnion = !arithData &&
									  unionData &&
									 !classStructData &&
									 !funcData &&
									 !memFuncPttrData &&
									 !enumData;

			constexpr bool isClassOrStruct = !arithData &&
											 !unionData &&
											  classStructData &&
											 !funcData &&
											 !memFuncPttrData &&
											 !enumData;

			constexpr bool isPttr = !arithData &&
									!unionData &&
									!classStructData &&
									 funcData &&
									!memFuncPttrData &&
									!enumData;

			constexpr bool isMemFuncPttr = !arithData &&
										   !unionData &&
										   !classStructData &&
										   !funcData &&
										    memFuncPttrData &&
										   !enumData;

			constexpr bool isEnum = !arithData &&
									!unionData &&
									!classStructData &&
									!funcData &&
									!memFuncPttrData &&
									 enumData;

			auto streams = std::make_tuple(printStreamPttr, logFileStreamPttr);
			auto stream = std::get<(int)dest>(streams);
			constexpr bool loggingToFile = std::is_same<decltype(stream), decltype(logFileStreamPttr)>::value;
			if constexpr (loggingToFile) { logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app); }
			if constexpr(isArith)
			{ *stream << "logging " << typeid(loggableType).name() << " with value " << dataLogging << '\n'; }
			else if constexpr (isUnion)
			{
				*stream << "\nlogging union with type-id " << typeid(loggableType).name() << '\n';
				*stream << "logging union stack-address " << &dataLogging << '\n';
				*stream << "no further details available" << '\n';
			}
			else if constexpr (isClassOrStruct)
			{
				*stream << "\nlogging struct/class with type-id " << typeid(loggableType).name() << '\n';
				*stream << "logging struct/class stack-address " << &dataLogging << '\n';
				*stream << "no further details available" << '\n';
			}
			else if constexpr (isPttr)
			{
				*stream << "\nlogging global function with type-id " << typeid(loggableType).name() << '\n';
				*stream << "logging function stack-address " << dataLogging << '\n';
				*stream << "no further details available" << '\n';
			}
			else if constexpr (isMemFuncPttr)
			{
				*stream << "\nlogging member function with type-id " << typeid(loggableType).name() << '\n';
				*stream << "logging function stack-address " << dataLogging << '\n';
				*stream << "no further details available" << '\n';
			}
			else if constexpr (isEnum)
			{
				*stream << "\nlogging enumeration with type-id " << typeid(loggableType).name() << " and numeric value " << (u8Byte)dataLogging << '\n';
				*stream << "logging enumeration stack-address " << &dataLogging << '\n';
				*stream << "no further details available" << '\n';
			}
			else
			{ *stream << "Sorry! Athru is only able to log objects with arithmetic, union, struct/class, function-pointer, or enum type" << '\n'; }
			*stream << "labelled as: " << label << '\n' << '\n';
			if constexpr (!loggingToFile) { ConsolePrinter::OutputText(stream); }
			else if constexpr (loggingToFile) { logFileStreamPttr->close(); }
		}

		// Logging for references-to-types (i.e. any case where [dataLogging] *is* a
		// pointer to data of type [loggableType])
		template<DESTINATIONS dest = DESTINATIONS::CONSOLE, typename loggableType>
		void Log(loggableType* dataLogging, const char* label = "(unlabelled)")
		{
			auto streams = std::make_tuple(printStreamPttr, logFileStreamPttr);
			auto stream = std::get<(int)dest>(streams);
			constexpr bool loggingToFile = std::is_same<decltype(stream), decltype(logFileStreamPttr)>::value;
			if (loggingToFile) { logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app); }
			if (dataLogging != nullptr)
			{
				constexpr bool isCString = (std::is_same<loggableType, const char>{});
				constexpr bool arithData = std::is_arithmetic<loggableType>{};
				constexpr bool unionData = std::is_union<loggableType>{};
				constexpr bool classStructData = std::is_class<loggableType>{};
				constexpr bool funcData = std::is_function<loggableType>{};
				constexpr bool memFuncPttrData = std::is_member_function_pointer<loggableType>{};
				constexpr bool enumData = std::is_enum<loggableType>{};
				if constexpr (arithData &&
							  !unionData &&
							  !classStructData &&
							  !funcData &&
							  !memFuncPttrData &&
							  !enumData)
				{
					if (!isCString)
					{ *stream << "logging " << typeid(loggableType).name() << " at " << dataLogging << " with value " << *dataLogging << '\n'; }
					else
					{
						*stream << "c-style string at " << (void*)dataLogging << ':' << '\n';
						*stream << dataLogging << '\n';
					}
				}
				else if constexpr (!arithData &&
								   unionData ||
								   classStructData &&
								   !funcData &&
								   !memFuncPttrData &&
								   !enumData)
				{
					*stream << "logging " << typeid(loggableType).name() << " at " << dataLogging << " " << '\n';
					*stream << "no further details available" << '\n';
				}
				else if (!arithData &&
						 !unionData &&
						 !classStructData &&
						 funcData &&
						 !memFuncPttrData &&
						 !enumData)
				{
					*stream << "logging global function with type-id " << typeid(loggableType).name() << " at " << dataLogging << '\n';
					*stream << "no further details available" << '\n';
				}
				else if (arithData &&
						 !unionData &&
						 !classStructData &&
						 !funcData &&
						 memFuncPttrData &&
						 !enumData)
				{
					*stream << "logging member function with type-id " << typeid(loggableType).name() << " at " << dataLogging << '\n';
					*stream << "no further details available" << '\n';
				}
				else if constexpr (!arithData &&
								   !unionData &&
								   !classStructData &&
								   !funcData &&
								   !memFuncPttrData &&
								   enumData)
				{
					*stream << "logging enumeration with type-id " << typeid(loggableType).name() << " and numeric value " << (u8Byte)*dataLogging << " at " << dataLogging << '\n';
					*stream << "no further details available" << '\n';
				}
				else
				{
					*stream << "Logging " << typeid(loggableType).name() << " pointer" << " with address " << dataLogging << '\n';
					*stream << "no further details available" << '\n';
				}
			}
			else
			{ *stream << "Unknown data stored at the null address (0x0000000000000000 in 64-bit, 0x00000000 in 32-bit)" << '\n'; }
			*stream << "labelled as: " << label << '\n' << '\n';
			if constexpr (!loggingToFile) { ConsolePrinter::OutputText(stream); }
			else if constexpr (loggingToFile) { logFileStreamPttr->close(); }
		}

		// Logging non-pointer arrays
		template<typename loggableType> void LogArray(loggableType* dataLogging, short arrayLength, DESTINATIONS destination,
													  const char* label = "(unlabelled)")
		{
			std::string msg = std::string("Logging array of type ");
			msg.append(typeid(loggableType).name());
			Log(msg.c_str(), destination, label);
			for (u2Byte i = 0; i < arrayLength; i += 1)
			{ Log(dataLogging[i], destination); }
		}

		// Logging arrays-of-pointers
		template<typename loggableType> void LogArray(loggableType** dataLogging, short arrayLength, DESTINATIONS destination,
													  const char* label = "(unlabelled)")
		{
			std::string msg = std::string("Logging array of type ");
			msg.append(typeid(loggableType).name());
			Log(msg.c_str(), destination, label);
			for (u2Byte i = 0; i < arrayLength; i += 1)
			{ Log(dataLogging[i], destination); }
		}

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		struct ConsolePrinter
		{
			public:
				static void OutputText(std::ostringstream* cStringStream)
				{
					strcpy_s(outputString, cStringStream->str().c_str());
					OutputDebugStringA(outputString);
					cStringStream->str("");
				}

			private:
				static char outputString[8191];
		};

		char* logFilePath;
		std::fstream* logFileStreamPttr;
		std::ostringstream* printStreamPttr;
};