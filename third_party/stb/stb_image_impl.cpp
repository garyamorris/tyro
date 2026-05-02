// Single translation unit that pulls in the stb_image implementation.
// Other code includes "stb_image.h" without the implementation macro.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO_OPTIONAL_FAIL_REASON
#include "stb_image.h"
