#pragma once

#include <iostream>

#define LEVEL_TRACE 5
#define LEVEL_DEBUG 3
#define LEVEL_ERROR 1
#define LOG_LEVEL LEVEL_TRACE

#if LOG_LEVEL >= LEVEL_TRACE
#define TRACE(m) std::cout << m << std::endl;
#else
#define TRACE(m)
#endif

#if LOG_LEVEL >= LEVEL_DEBUG
#define DEBUG(m) std::cout << m << std::endl;
#else
#define DEBUG(m)
#endif
