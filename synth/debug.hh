#pragma once
#include <iostream>
#include <sstream>

// #define DEBUG_MODE

#ifdef DEBUG_MODE
#define debug(s) std::cerr << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << "(...): " << s << "\n";
#else
#define debug(stream) \
    do {              \
    } while (false)
#endif
