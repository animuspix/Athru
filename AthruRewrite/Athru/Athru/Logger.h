#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include "Typedefs.h"
#include "ConsolePrinter.h"
#include "Dispatcher.h"
#include "leakChecker.h"

class Logger
{
	public:
		Logger(char* loggingTo)
		{
			logFilePath = loggingTo;

			// Streams ONLY work if they're allocated on an external heap;
			// ask SO what the fruit is going on there
			logFileStreamPttr = DEBUG_NEW std::fstream();
			logFileStreamPttr->open(logFilePath, std::ios::out);
			(*logFileStreamPttr) << "Note:" << '\n';
			(*logFileStreamPttr) << "Unions + structs/classes are too complicated to easily log members," << '\n';
			(*logFileStreamPttr) << "and it's impossible to easily log the names of enum values without" << '\n';
			(*logFileStreamPttr) << "lots of boilerplate code. This has resulted in the following:" << '\n' << '\n';;
			(*logFileStreamPttr) << "- Athru will only ever log the address + type-id of unions and" << '\n';
			(*logFileStreamPttr) << "  avoid describing their members" << '\n' << '\n';
			(*logFileStreamPttr) << "- Athru will only ever log the address + type-id of structs/classes" << '\n';
			(*logFileStreamPttr) << "  and avoid describing their members" << '\n' << '\n';
			(*logFileStreamPttr) << "- Athru will not attempt to log enums; any enum values that you want" << '\n';
			(*logFileStreamPttr) << "  to log should be cast into an arithmetic type beforehand, and enum" << '\n';
			(*logFileStreamPttr) << "  names must be stringified before being passed to the logger" << '\n' << '\n';
			(*logFileStreamPttr) << "Thank you for reading :)" << '\n' << '\n';
			logFileStreamPttr->close();

			printStreamPttr = DEBUG_NEW std::ostringstream();
			*printStreamPttr << "Note:" << '\n';
			*printStreamPttr << "Unions + structs/classes are too complicated to easily log members," << '\n';
			*printStreamPttr << "and it's impossible to easily log the names of enum values without" << '\n';
			*printStreamPttr << "lots of boilerplate code. This has resulted in the following:" << '\n' << '\n';
			*printStreamPttr << "- Athru will only ever log the address + type-id of unions and" << '\n';
			*printStreamPttr << "  avoid describing their members" << '\n' << '\n';
			*printStreamPttr << "- Athru will only ever log the address + type-id of structs/classes" << '\n';
			*printStreamPttr << "  and avoid describing their members" << '\n' << '\n';
			*printStreamPttr << "- Athru will not attempt to log enums; any enum values that you want" << '\n';
			*printStreamPttr << "  to log should be cast into an arithmetic type beforehand, and enum" << '\n';
			*printStreamPttr << "  names must be stringified before being passed to the logger" << '\n' << '\n';
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

		template<typename loggableType> void Log(loggableType dataLogging, DESTINATIONS destination)
		{
			Dispatch(std::is_arithmetic<loggableType>{})
			(
				[&](auto&& value)
				{
					if (destination == DESTINATIONS::CONSOLE)
					{
						*printStreamPttr << "logging " << typeid(loggableType).name() << " with value " << dataLogging << '\n';
					}

					else
					{
						logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
						(*logFileStreamPttr) << "logging " << typeid(loggableType).name() << " with value " << dataLogging << '\n';
						logFileStreamPttr->close();
					}
				},

				[&](auto&& value)
				{
					Dispatch(std::is_union<loggableType>{})
					(
						[&](auto&& value)
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
								(*logFileStreamPttr) << "logging union with type-id " << typeid(loggableType).name() << '\n';;
								(*logFileStreamPttr) << "logging union stack-address " << &dataLogging << '\n';
								(*logFileStreamPttr) << "no further details available" << '\n' << '\n';
								logFileStreamPttr->close();
							}
						},

						[&](auto&& value)
						{
							Dispatch(std::is_class<loggableType>{})
							(
								[&](auto&& value)
								{
									if (destination == DESTINATIONS::CONSOLE)
									{
										*printStreamPttr << "\nlogging class with type-id " << typeid(loggableType).name() << '\n';
										*printStreamPttr << "logging class stack-address " << &dataLogging << '\n';
										*printStreamPttr << "no further details available" << '\n' << '\n';
									}

									else
									{
										logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
										(*logFileStreamPttr) << "\nlogging class with type-id " << typeid(loggableType).name() << '\n';
										(*logFileStreamPttr) << "logging class stack-address " << &dataLogging << '\n';
										(*logFileStreamPttr) << "no further details available" << '\n' << '\n';
										logFileStreamPttr->close();
									}
								},

								[&](auto&& value)
								{
									Dispatch(std::is_pointer<loggableType>{})
									(
										[&](auto&& value)
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
												(*logFileStreamPttr) << "\nlogging global function with type-id " << typeid(loggableType).name() << '\n';
												(*logFileStreamPttr) << "logging function stack-address " << dataLogging << '\n';
												(*logFileStreamPttr) << "no further details available" << '\n' << '\n';
												logFileStreamPttr->close();
											}
										},

										[&](auto&& value)
										{
											Dispatch(std::is_member_function_pointer<loggableType>{})
											(
												[&](auto&& value)
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
														(*logFileStreamPttr) << "\nlogging member function with type-id " << typeid(loggableType).name() << '\n';
														(*logFileStreamPttr) << "logging function stack-address " << dataLogging << '\n';
														(*logFileStreamPttr) << "no further details available" << '\n' << '\n';
														logFileStreamPttr->close();
													}
												},

												[&](auto&& value)
												{
													if (destination == DESTINATIONS::CONSOLE)
													{
														*printStreamPttr << "Sorry! Athru is only able to log objects with arithmetic, union, class, or function type" << '\n';
													}

													else
													{
														logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
														(*logFileStreamPttr) << "Sorry! Athru is only able to log objects with arithmetic, union, class, or function type" << '\n';
														logFileStreamPttr->close();
													}
												}
											)
											(dataLogging);
										}
									)
									(dataLogging);
								}
							)
							(dataLogging);
						}
					)
					(dataLogging);
				}
			)
			(dataLogging);

			ConsolePrinter::OutputText(printStreamPttr);
		}

		// Logging pointers to CMD
		template<typename loggableType> void Log(loggableType* dataLogging, DESTINATIONS destination)
		{
			if (dataLogging != nullptr)
			{
				bool isCString = (typeid(loggableType) == typeid(char)) && std::is_const<loggableType>::value;

				if (destination == DESTINATIONS::CONSOLE)
				{
					Dispatch(std::is_arithmetic<loggableType>{})
					(
						[&](auto&& value)
						{
							if (!isCString)
							{
								*printStreamPttr << "logging " << typeid(loggableType).name() << " at " << &dataLogging << " with value " << *dataLogging << '\n';
							}

							else
							{
								*printStreamPttr << "c-style string at " << &dataLogging << ':' << '\n';
								*printStreamPttr << dataLogging << '\n';
							}
						},

						[&](auto&& value)
						{
							*printStreamPttr << "\nlogging " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
							*printStreamPttr << "no further details available" << '\n' << '\n';
						}
					)
					(dataLogging);

					ConsolePrinter::OutputText(printStreamPttr);
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					Dispatch(std::is_arithmetic<loggableType>{})
					(
						[&](auto&& value)
						{
							if (!isCString)
							{
								(*logFileStreamPttr) << "logging " << typeid(loggableType).name() << " at " << &dataLogging << " with value " << *dataLogging << '\n';
							}

							else
							{
								(*logFileStreamPttr) << "c-style string at " << &dataLogging << ':' << '\n';
								(*logFileStreamPttr) << dataLogging << '\n';
							}
						},

						[&](auto&& value)
						{
							Dispatch(std::is_class<loggableType>{})
							(
								[&](auto&& value)
								{
									(*logFileStreamPttr) << "\nlogging " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
									(*logFileStreamPttr) << "no further details available" << '\n';
								},

								[&](auto&& value)
								{
									Dispatch(std::is_union<loggableType>{})
									(
										[&](auto&& value)
										{
											(*logFileStreamPttr) << "\nlogging " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
											(*logFileStreamPttr) << "no further details available" << '\n';
										},

										[&](auto&& value)
										{
											Dispatch(std::is_enum<loggableType>{})
											(
												[&](auto&& value)
												{
													(*logFileStreamPttr) << "\nlogging " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
													(*logFileStreamPttr) << "no further details available" << '\n';
												},

												[&](auto&& value)
												{
													(*logFileStreamPttr) << "\nlogging global function with type-id " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
													(*logFileStreamPttr) << "no further details available" << '\n';
												}
											)(dataLogging);
										}
									)(dataLogging);
								}
							)(dataLogging);
						}
					)
					(dataLogging);

					logFileStreamPttr->close();
				}
			}

			else
			{
				if (destination == DESTINATIONS::CONSOLE)
				{
					*printStreamPttr << "Unknown data stored at the null address (0x0000000000000000)" << '\n';
					ConsolePrinter::OutputText(printStreamPttr);
				}

				else
				{
					logFileStreamPttr->open(logFilePath, std::fstream::out | std::fstream::app);
					(*logFileStreamPttr) << "Unknown data stored at the null address (0x0000000000000000)" << '\n';
					logFileStreamPttr->close();
				}
			}
		}

		// Logging non-pointer arrays to CMD
		template<typename loggableType> void LogArray(loggableType* dataLogging, short arrayLength, DESTINATIONS destination)
		{
			for (twoByteUnsigned i = 0; i < arrayLength; i += 1)
			{
				Log(dataLogging[i], destination);
			}
		}

		// Logging pointer-arrays to CMD
		template<typename loggableType> void LogArray(loggableType** dataLogging, short arrayLength, DESTINATIONS destination)
		{
			for (twoByteUnsigned i = 0; i < arrayLength; i += 1)
			{
				Log(dataLogging[i], destination);
			}
		}

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		char* logFilePath;
		std::fstream* logFileStreamPttr;
		std::ostringstream* printStreamPttr;
};