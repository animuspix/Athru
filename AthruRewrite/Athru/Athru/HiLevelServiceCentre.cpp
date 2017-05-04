#include "HiLevelServiceCentre.h"

StackAllocator* HiLevelServiceCentre::stackAllocatorPttr = nullptr;
Logger* HiLevelServiceCentre::loggerPttr = nullptr;
Input* HiLevelServiceCentre::inputPttr = nullptr;
Application* HiLevelServiceCentre::appPttr = nullptr;
Graphics* HiLevelServiceCentre::graphicsPttr = nullptr;
TextureManager* HiLevelServiceCentre::textureManagerPttr = nullptr;
RenderManager* HiLevelServiceCentre::renderManagerPttr = nullptr;
SceneManager* HiLevelServiceCentre::sceneManagerPttr = nullptr;