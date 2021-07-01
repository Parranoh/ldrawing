#ifdef DEBUG_PRINT
#include <iostream>
#undef DEBUG_PRINT
#define DEBUG_PRINT(s) do { std::cerr << __FILE__ << ':' << __LINE__ << ":\t" << s << std::endl; } while (false)
#define DEBUG
#else
#define DEBUG_PRINT(s) do {} while (false)
#endif
