#pragma once

#include <sstream>
#include <fstream>
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
		template<typename loggableType> void Log(loggableType dataLogging, DESTINATIONS destination = DESTINATIONS::CONSOLE)
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

			if constexpr(isArith)
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "logging " << typeid(loggableType).name() << " with value " << dataLogging << '\n';
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					*logFileStreamPttr << "logging " << typeid(loggableType).name() << " with value " << dataLogging << '\n';
					logFileStreamPttr->close();
				}
			}

			else if constexpr (isUnion)
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "\nlogging union with type-id " << typeid(loggableType).name() << '\n';
					*printStreamPttr << "logging union stack-address " << &dataLogging << '\n';
					*printStreamPttr << "no further details available" << '\n' << '\n';
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					*logFileStreamPttr << "logging union with type-id " << typeid(loggableType).name() << '\n';;
					*logFileStreamPttr << "logging union stack-address " << &dataLogging << '\n';
					*logFileStreamPttr << "no further details available" << '\n' << '\n';
					logFileStreamPttr->close();
				}
			}

			else if constexpr (isClassOrStruct)
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "\nlogging struct/class with type-id " << typeid(loggableType).name() << '\n';
					*printStreamPttr << "logging struct/class stack-address " << &dataLogging << '\n';
					*printStreamPttr << "no further details available" << '\n' << '\n';
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					*logFileStreamPttr << "\nlogging struct/class with type-id " << typeid(loggableType).name() << '\n';
					*logFileStreamPttr << "logging struct/class stack-address " << &dataLogging << '\n';
					*logFileStreamPttr << "no further details available" << '\n' << '\n';
					logFileStreamPttr->close();
				}

			}

			else if constexpr (isPttr)
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "\nlogging global function with type-id " << typeid(loggableType).name() << '\n';
					*printStreamPttr << "logging function stack-address " << dataLogging << '\n';
					*printStreamPttr << "no further details available" << '\n' << '\n';
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					*logFileStreamPttr << "\nlogging global function with type-id " << typeid(loggableType).name() << '\n';
					*logFileStreamPttr << "logging function stack-address " << dataLogging << '\n';
					*logFileStreamPttr << "no further details available" << '\n' << '\n';
					logFileStreamPttr->close();
				}
			}

			else if constexpr (isMemFuncPttr)
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "\nlogging member function with type-id " << typeid(loggableType).name() << '\n';
					*printStreamPttr << "logging function stack-address " << dataLogging << '\n';
					*printStreamPttr << "no further details available" << '\n' << '\n';
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					*logFileStreamPttr << "\nlogging member function with type-id " << typeid(loggableType).name() << '\n';
					*logFileStreamPttr << "logging function stack-address " << dataLogging << '\n';
					*logFileStreamPttr << "no further details available" << '\n' << '\n';
					logFileStreamPttr->close();
				}
			}

			else if constexpr (isEnum)
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "\nlogging enumeration with type-id " << typeid(loggableType).name() << " and numeric value " << (eightByteUnsigned)dataLogging << '\n';
					*printStreamPttr << "logging enumeration stack-address " << &dataLogging << '\n';
					*printStreamPttr << "no further details available" << '\n' << '\n';
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					*logFileStreamPttr << "\nlogging enumeration with type-id " << typeid(loggableType).name() << '\n';
					*logFileStreamPttr << "logging enumeration stack-address " << &dataLogging << '\n';
					*logFileStreamPttr << "no further details available" << '\n' << '\n';
					logFileStreamPttr->close();
				}
			}

			else
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "Sorry! Athru is only able to log objects with arithmetic, union, struct/class, function-pointer, or enum type" << '\n';
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					*logFileStreamPttr << "Sorry! Athru is only able to log objects with arithmetic, union, struct/class, function-pointer, or enum type" << '\n';
					logFileStreamPttr->close();
				}
			}

			ConsolePrinter::OutputText(printStreamPttr);
		}

		// Logging for references-to-types (i.e. any case where [dataLogging] *is* a
		// pointer to data of type [loggableType])
		template<typename loggableType> void Log(loggableType* dataLogging, DESTINATIONS destination = DESTINATIONS::CONSOLE)
		{
			if (dataLogging != nullptr)
			{
				constexpr bool isCString = (std::is_same<loggableType, const char>{});
				if (destination == DESTINATIONS::CONSOLE)
				{
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
						{
							*printStreamPttr << "logging " << typeid(loggableType).name() << " at " << dataLogging << " with value " << *dataLogging << '\n';
						}

						else
						{
							*printStreamPttr << "c-style string at " << (void*)dataLogging << ':' << '\n';
							*printStreamPttr << dataLogging << '\n';
						}
					}

					else if constexpr (!arithData &&
									    unionData ||
									    classStructData &&
									   !funcData &&
									   !memFuncPttrData &&
									   !enumData)
					{
						*printStreamPttr << "\nlogging " << typeid(loggableType).name() << " at " << dataLogging << '\n';
						*printStreamPttr << "no further details available" << '\n';
					}

					else if (!arithData &&
							 !unionData &&
							 !classStructData &&
							  funcData &&
							 !memFuncPttrData &&
							 !enumData)
					{
						*printStreamPttr << "\nlogging global function with type-id " << typeid(loggableType).name() << " at " << dataLogging << '\n';
						*printStreamPttr << "no further details available" << '\n';
					}

					else if (arithData &&
							 !unionData &&
							 !classStructData &&
							 !funcData &&
							  memFuncPttrData &&
							 !enumData)
					{
						*printStreamPttr << "\nlogging member function with type-id " << typeid(loggableType).name() << " at " << dataLogging << '\n';
						*printStreamPttr << "no further details available" << '\n' << '\n';
					}

					else if constexpr (!arithData &&
									   !unionData &&
									   !classStructData &&
									   !funcData &&
									   !memFuncPttrData &&
									    enumData)
					{
						*printStreamPttr << "\nlogging enumeration with type-id " << typeid(loggableType).name() << " and numeric value " << (eightByteUnsigned)*dataLogging << " at " << dataLogging << '\n';
						*printStreamPttr << "no further details available" << '\n' << '\n';
					}

					else
					{
						*printStreamPttr << "\nLogging " << typeid(loggableType).name() << " pointer" << " with address " << dataLogging << '\n';
						*printStreamPttr << "no further details available" << '\n' << '\n';
					}

					ConsolePrinter::OutputText(printStreamPttr);
				}

				else
				{
					constexpr bool arithData = std::is_arithmetic<loggableType>{};
					constexpr bool unionData = std::is_union<loggableType>{};
					constexpr bool classStructData = std::is_class<loggableType>{};
					constexpr bool funcData = std::is_function<loggableType>{};
					constexpr bool memFuncPttrData = std::is_member_function_pointer<loggableType>{};
					constexpr bool enumData = std::is_enum<loggableType>{};

					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					if constexpr (arithData &&
								  !unionData &&
								  !classStructData &&
								  !funcData &&
								  !memFuncPttrData &&
								  !enumData)
					{
						if (!isCString)
						{
							*logFileStreamPttr << "logging " << typeid(loggableType).name() << " at " << dataLogging << " with value " << *dataLogging << '\n';
						}

						else
						{
							*logFileStreamPttr << "c-style string at " << (void*)dataLogging << ':' << '\n';
							*logFileStreamPttr << dataLogging << '\n';
						}
					}

					else if constexpr (!arithData &&
									    unionData ||
									    classStructData &&
									   !funcData &&
									   !memFuncPttrData &&
									   !enumData)
					{
						*logFileStreamPttr << "\nlogging " << typeid(loggableType).name() << " at " << dataLogging << '\n';
						*logFileStreamPttr << "no further details available" << '\n';
					}

					else if constexpr (!arithData &&
									   !unionData &&
									   !classStructData &&
									    funcData &&
									   !memFuncPttrData &&
									   !enumData)
					{
						*logFileStreamPttr << "\nlogging global function with type-id " << typeid(loggableType).name() << " at " << dataLogging << '\n';
						*logFileStreamPttr << "no further details available" << '\n';
					}

					else if (!arithData &&
							 !unionData &&
							 !classStructData &&
							 !funcData &&
							  memFuncPttrData &&
							 !enumData)
					{
						*logFileStreamPttr << "\nlogging member function with type-id " << typeid(loggableType).name() << " at " << dataLogging << '\n';
						*logFileStreamPttr << "no further details available" << '\n' << '\n';
					}

					else if constexpr (!arithData &&
									   !unionData &&
									   !classStructData &&
									   !funcData &&
									   !memFuncPttrData &&
									    enumData)
					{
						*logFileStreamPttr << "\nlogging enumeration with type-id " << typeid(loggableType).name() << " and numeric value " << (eightByteUnsigned)*dataLogging << " at " << dataLogging << '\n';
						*logFileStreamPttr << "no further details available" << '\n' << '\n';
					}

					else
					{
						*logFileStreamPttr << "\nLogging " << typeid(loggableType).name() << " pointer" << " with address " << dataLogging << '\n';
						*logFileStreamPttr << "no further details available" << '\n' << '\n';
					}

					logFileStreamPttr->close();
				}
			}

			else
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "Unknown data stored at the null address (0x0000000000000000 in 64-bit, 0x00000000 in 32-bit)" << '\n';
					ConsolePrinter::OutputText(printStreamPttr);
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					*logFileStreamPttr << "Unknown data stored at the null address (0x0000000000000000 in 64-bit, 0x00000000 in 32-bit)" << '\n';
					logFileStreamPttr->close();
				}
			}
		}

		// Logging non-pointer arrays
		template<typename loggableType> void LogArray(loggableType* dataLogging, short arrayLength, DESTINATIONS destination)
		{
			std::string msg = std::string("Logging array of type ");
			msg.append(typeid(loggableType).name());
			Log(msg.c_str(), destination);
			for (twoByteUnsigned i = 0; i < arrayLength; i += 1)
			{
				Log(dataLogging[i], destination);
			}
		}

		// Logging arrays-of-pointers
		template<typename loggableType> void LogArray(loggableType** dataLogging, short arrayLength, DESTINATIONS destination)
		{
			std::string msg = std::string("Logging array of type ");
			msg.append(typeid(loggableType).name());
			Log(msg.c_str(), destination);
			for (twoByteUnsigned i = 0; i < arrayLength; i += 1)
			{
				Log(dataLogging[i], destination);
			}
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