#include <iostream>
#include <sstream>
#include <fstream>
#include "Typedefs.h"
#include "ConsolePrinter.h"
#include "Service.h"

#include "Dispatcher.h"

#pragma once

class Logger : public Service
{
	public:
		Logger(char* loggingTo) : Service("Logger")
		{
			logFilePath = loggingTo;

			logFileStream = std::fstream();
			logFileStream.open(logFilePath, std::ios::out);

			logFileStream << "Note:" << '\n';
			logFileStream << "Unions + structs/classes are too complicated to easily log members," << '\n';
			logFileStream << "and it's impossible to easily log the names of enum values without" << '\n';
			logFileStream << "lots of boilerplate code. This has resulted in the following:" << '\n' << '\n';;
			logFileStream << "- Athru will only ever log the address + type-id of unions and" << '\n';
			logFileStream << "  avoid describing their members" << '\n' << '\n';
			logFileStream << "- Athru will only ever log the address + type-id of structs/classes" << '\n';
			logFileStream << "  and avoid describing their members" << '\n' << '\n';
			logFileStream << "- Athru will not attempt to log enums; any enum values that you want" << '\n';
			logFileStream << "  to log should be cast into an arithmetic type beforehand, and enum" << '\n';
			logFileStream << "  names must be stringified before being passed to the logger" << '\n' << '\n';
			logFileStream << "Thank you for reading :)" << '\n' << '\n';
			logFileStream.close();

			printStreamPttr = new std::ostringstream();
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
						logFileStream.open(logFilePath, std::fstream::out | std::fstream::app);
						logFileStream << "logging " << typeid(loggableType).name() << " with value " << dataLogging << '\n';
						logFileStream.close();
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
								logFileStream.open(logFilePath, std::fstream::out | std::fstream::app);
								logFileStream << "logging union with type-id " << typeid(loggableType).name() << '\n';;
								logFileStream << "logging union stack-address " << &dataLogging << '\n';
								logFileStream << "no further details available" << '\n' << '\n';
								logFileStream.close();
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
										logFileStream.open(logFilePath, std::fstream::out | std::fstream::app);
										logFileStream << "\nlogging class with type-id " << typeid(loggableType).name() << '\n';
										logFileStream << "logging class stack-address " << &dataLogging << '\n';
										logFileStream << "no further details available" << '\n' << '\n';
										logFileStream.close();
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
												logFileStream.open(logFilePath, std::fstream::out | std::fstream::app);
												logFileStream << "\nlogging global function with type-id " << typeid(loggableType).name() << '\n';
												logFileStream << "logging function stack-address " << dataLogging << '\n';
												logFileStream << "no further details available" << '\n' << '\n';
												logFileStream.close();
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
														logFileStream.open(logFilePath, std::fstream::out | std::fstream::app);
														logFileStream << "\nlogging member function with type-id " << typeid(loggableType).name() << '\n';
														logFileStream << "logging function stack-address " << dataLogging << '\n';
														logFileStream << "no further details available" << '\n' << '\n';
														logFileStream.close();
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
														logFileStream.open(logFilePath, std::fstream::out | std::fstream::app);
														logFileStream << "Sorry! Athru is only able to log objects with arithmetic, union, class, or function type" << '\n';
														logFileStream.close();
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
					logFileStream.open(logFilePath, std::fstream::out | std::fstream::app);
					Dispatch(std::is_arithmetic<loggableType>{})
					(
						[&](auto&& value)
						{
							if (!isCString)
							{
								logFileStream << "logging " << typeid(loggableType).name() << " at " << &dataLogging << " with value " << *dataLogging << '\n';
							}

							else
							{
								logFileStream << "c-style string at " << &dataLogging << ':' << '\n';
								logFileStream << dataLogging << '\n';
							}
						},

						[&](auto&& value)
						{
							Dispatch(std::is_class<loggableType>{})
							(
								[&](auto&& value)
								{
									logFileStream << "\nlogging " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
									logFileStream << "no further details available" << '\n';
								},

								[&](auto&& value)
								{
									Dispatch(std::is_union<loggableType>{})
									(
										[&](auto&& value)
										{
											logFileStream << "\nlogging " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
											logFileStream << "no further details available" << '\n';
										},

										[&](auto&& value)
										{
											Dispatch(std::is_enum<loggableType>{})
											(
												[&](auto&& value)
												{
													logFileStream << "\nlogging " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
													logFileStream << "no further details available" << '\n';
												},

												[&](auto&& value)
												{
													logFileStream << "\nlogging global function with type-id " << typeid(loggableType).name() << " at " << &dataLogging << '\n';
													logFileStream << "no further details available" << '\n';
												}
											)(dataLogging);
										}
									)(dataLogging);
								}
							)(dataLogging);
						}
					)
					(dataLogging);

					logFileStream.close();
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
					logFileStream.open(logFilePath, std::fstream::out | std::fstream::app);
					logFileStream << "Unknown data stored at the null address (0x0000000000000000)" << '\n';
					logFileStream.close();
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

	private:
		char* logFilePath;
		std::fstream logFileStream;
		std::ostringstream* printStreamPttr;
};
