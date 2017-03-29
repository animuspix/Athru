#include "ServiceCentre.h"

StackAllocator* ServiceCentre::stackAllocatorPttr = nullptr;
Logger* ServiceCentre::loggerPttr = nullptr;
Input* ServiceCentre::inputPttr = nullptr;
Application* ServiceCentre::appPttr = nullptr;
Graphics* ServiceCentre::graphicsPttr = nullptr;
TextureManager* ServiceCentre::textureManagerPttr = nullptr;
RenderManager* ServiceCentre::renderManagerPttr = nullptr;
SceneManager* ServiceCentre::sceneManagerPttr = nullptr;