#include "Application.h"
#include "StackAllocator.h"
#include "Logger.h"
#include "Input.h"
#include "Graphics.h"
#include "ServiceCentre.h"

#include <typeinfo.h>

ServiceCentre* ServiceCentre::self = nullptr;
StackAllocator* ServiceCentre::stackAllocatorPtr = nullptr;
Logger* ServiceCentre::loggerPtr = nullptr;
Input* ServiceCentre::inputPtr = nullptr;
Application* ServiceCentre::appPtr = nullptr;
Graphics* ServiceCentre::graphicsPtr = nullptr;

ServiceCentre::ServiceCentre()
{
	// Attempt to create and register the memory-management
	// service, then abandon ship if creation fails
	stackAllocatorPtr = new StackAllocator();

	// Attempt to create and register the logging
	// service
	loggerPtr = new Logger("log.txt");

	// Attempt to create and register the primary input service
	inputPtr = new Input();

	// Create and register graphics support services here

	// Attempt to create and register the primary graphics
	// service
	graphicsPtr = new Graphics();

	// Create and register additional application-support
	// services here

	// Attempt to create and register the application
	appPtr = new Application();

	// Create and register additional, independent services over here

	// Place a marker to separate long-term, app-duration
	// memory from short-term memory that will be written and over-written on
	// the fly
	stackAllocatorPtr->SetMarker();
}

ServiceCentre::~ServiceCentre()
{
	CleanUp();
	delete stackAllocatorPtr;
	stackAllocatorPtr = nullptr;
}

void ServiceCentre::CleanUp()
{
	loggerPtr->~Logger();
	inputPtr->~Input();
	graphicsPtr->~Graphics();
	appPtr->~Application();

	loggerPtr = nullptr;
	inputPtr = nullptr;
	graphicsPtr = nullptr;
	appPtr = nullptr;
}