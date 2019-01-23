#include "AppGlobals.h"

// Anything that can't be evaluated at compile time goes here...
std::chrono::steady_clock::time_point TimeStuff::lastFrameTime = std::chrono::steady_clock::now();
u4Byte TimeStuff::frameCtr = 0;