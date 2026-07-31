#pragma once
// Force PCH Rebuild

#ifdef HIMII_PLATFORM_WINDOWS
	#ifndef NOMINMAX
	#define NOMINMAX
#endif
#endif

#ifdef HIMII_PLATFORM_WINDOWS
#include <Windows.h>
#ifdef NEAR
#undef NEAR
#define NEAR
#endif
#ifdef FAR
#undef FAR
#define FAR
#endif
#undef near
#undef far
#endif

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <ctime>

#include <stdio.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "EngineCore/Core/Core.h"
#include "EngineCore/Core/Log.h"

#include "EngineCore/Instrument/Instrumentor.h"


