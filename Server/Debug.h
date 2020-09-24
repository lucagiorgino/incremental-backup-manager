#pragma once

#define _DEBUG 0

#if _DEBUG
#define DEBUG_PRINT(line) std::cout << (line);
#else
#define DEBUG_PRINT(line)
#endif

#define PRINT(line) std::cout << (line);
