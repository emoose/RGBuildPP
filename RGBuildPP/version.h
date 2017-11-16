#ifndef VERSIONH
#define VERSIONH

#define RGB_VER 5
#define RGB_BLD 420

#ifdef _XBOX
#define RGB_PLT "XENON"
#endif // _XBOX

#ifdef _WIN32
#define RGB_PLT "WIN32"
#else
#define RGB_PLT "*NUX" // we can hope
#endif // _WIN32

#define RGB_TODO "\n\
* IMPORTANT: bigblock (semi-supported, i think)\n\
* IMPORTANT: config.ini reading\n\
* IMPORTANT: guided mode (see tree folder, might need to change the format to support xebuild style stuff though)\n\
* IMPORTANT: bootloader patching - kinda done, supports rglp/patch binaries in format (2BL_B+4BL+KHV)\n\
(should try and use xenon-as/xenon-objcopy sources to compile them in-app)\n\
* IMPORTANT: xbox xex testing\n\
* IMPORTANT: readImageIni memory leaks\n\
\n\
* support for xebuild style ini (xex should be able to act as an xbox equivalent of it, so why not)\n\
* addon patches like xebuild\n\
(not sure how it does it, probably appends patches before final 0xFFFFFFFF in khv patch)\n\
* secured file crypto (extended/secdata/crl/dae/fcrt), needed for some stupid shit to do with LDV i think\n\
* better bootloader.isValid() function to check if crypto worked\n\
* ecc spare data stuff?\n\
* map command to show map of file e.g. header, smc, kv, bldrs, payloads, filesystems, files, mobile data and config blocks\
"
#endif // VERSIONH