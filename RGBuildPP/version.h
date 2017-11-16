#ifndef VERSIONH
#define VERSIONH

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// main version stuff
#define RGB_VER_MAJ 5
#define RGB_VER_MIN 1     // change this to 1 for public releases!
#define RGB_VER_BLD 420   // change this whenever you've made a difference

#define RGB_TODO "\n\
* IMPORTANT: bigblock - semi-supported, i think\n\
* IMPORTANT: bad blocks\n\
* IMPORTANT: config.ini reading\n\
* IMPORTANT: guided mode - see tree folder, might need to change the format to support xebuild style stuff though\n\
* IMPORTANT: bootloader patching - should try and use xenon-as/xenon-objcopy sources to compile them in-app\n\
* IMPORTANT: xbox xex testing\n\
* IMPORTANT: readImageIni memory leaks\n\
\n\
* support for xebuild style ini? - xex should be able to act as an xbox equivalent of it, so why not\n\
* addon patches like xebuild\n\
* secured file crypto (extended/secdata/crl/dae/fcrt), needed for some stupid shit to do with LDV i think\n\
* better bootloader.isValid() function to check if crypto worked\n\
* ecc spare data stuff?\n\
* map command to show map (offset:size:description) of file e.g. header, smc, kv, bldrs, payloads, filesystems, files, mobile data and config blocks\
"

// don't touch anything below here

#ifdef _XBOX
#define RGB_VER_PLT "360"
#define PATH_SEP "\\"
#else

#ifdef _WIN32
#define RGB_VER_PLT "x86"
#define PATH_SEP "\\"
#else
#define RGB_VER_PLT "gnu" // we can hope
#define PATH_SEP "/"
#endif // _WIN32

#endif

#ifdef _DEBUG
#define RGB_VER_TAG "chk"
#else
#define RGB_VER_TAG "fre"
#endif

#if !defined(MACHINENAME)
#define MACHINENAME "BUILDMSTR"
#endif

#if !defined(USERNAME)
#define USERNAME "ROOT"
#endif

#define RGB_VER_STR STR(RGB_VER_MAJ) "." STR(RGB_VER_MIN) "." STR(RGB_VER_BLD)
#define RGB_VER_STR_TAG RGB_VER_STR "." RGB_VER_PLT RGB_VER_TAG
#define RGB_VER_STR_FULL RGB_VER_STR_TAG "(" STR(USERNAME) "@" STR(MACHINENAME) ")"

const char rgbBuildStr[] = RGB_VER_STR;
const char rgbBuildStrTag[] = RGB_VER_STR_TAG;
const char rgbBuildStrFull[] = RGB_VER_STR_FULL;

#endif // VERSIONH
