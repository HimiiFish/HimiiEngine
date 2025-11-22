#pragma once

#ifdef HIMII_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		// See github.com/skypjack/entt/wiki/Frequently-Asked-Questions#warning-c4003-the-min-the-max-and-the-macro
	#define NOMINMAX
#endif
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

#include "Himii/Core/Core.h"
#include "Himii/Core/Log.h"

#include "Himii/Instrument/Instrumentor.h"

#ifdef HIMII_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
