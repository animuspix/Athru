#pragma once

// A C++ leak-tracker, using CRT hooks found on the Microsoft
// Developer Network; this implementation was found and originally
// shared by Sean Beyer at AIE

// [new] preproccessor redefinition edited by Paul Ferris to
// cooperate with [new]/[delete] operator overloading

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW
#endif

#ifndef _DEBUG
#define DEBUG_NEW new
#endif

#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>