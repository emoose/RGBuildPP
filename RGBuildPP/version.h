#ifndef VERSIONH
#define VERSIONH

#define RGB_VER 5
#define RGB_BLD 225

#ifdef _XBOX
#define RGB_PLT "XENON"
#endif // _XBOX

#ifdef _WIN32
#define RGB_PLT "WIN32"
#else
#define RGB_PLT "*NUX"
#endif // _WIN32

#define RGB_TODO "\n\
* save filesystem\n\
* save mobile files\n\
* ecc stuff\n\
* bigblock (semi-supported)\n\
* saving 6/7bl\n\
* bootloader patching"

#endif // VERSIONH