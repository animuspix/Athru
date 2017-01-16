#include "Application.h"
#include "StackAllocator.h"
#include "Logger.h"
#include "Input.h"
#include "Graphics.h"
#include "ServiceCentre.h"

#include <typeinfo.h>

StackAllocator* ServiceCentre::stackAllocatorPtr = nullptr;
Logger* ServiceCentre::loggerPtr = nullptr;
Input* ServiceCentre::inputPtr = nullptr;
Application* ServiceCentre::appPtr = nullptr;
Graphics* ServiceCentre::graphicsPtr = nullptr;

ServiceCentre::ServiceCentre()
{

}

ServiceCentre::~ServiceCentre()
{

}