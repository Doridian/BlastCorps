/* stb_image (public domain or MIT, port/third_party/stb), for resource
   packs: PNGs, and its zlib decoder for the zip's deflated members. */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_FAILURE_STRINGS
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "../third_party/stb/stb_image.h"
