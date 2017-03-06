#include "Application.h"
#include "StackAllocator.h"
#include "Logger.h"
#include "Input.h"
#include "Graphics.h"
#include "ServiceCentre.h"

StackAllocator* ServiceCentre::stackAllocatorPttr = nullptr;
Logger* ServiceCentre::loggerPttr = nullptr;
Input* ServiceCentre::inputPttr = nullptr;
Application* ServiceCentre::appPttr = nullptr;
Graphics* ServiceCentre::graphicsPttr = nullptr;
RenderManager* ServiceCentre::renderManagerPttr = nullptr;
SceneManager* ServiceCentre::sceneManagerPttr = nullptr;