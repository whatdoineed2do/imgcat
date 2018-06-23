#include "version.h"

namespace Imgcat
{

const char*  version()
{
#ifdef IMGCAT_VERSION
#define IMGCAT_STRM(x)  #x
#define IMGCAT_STR(x)   IMGCAT_STRM(x)
#define IMGCAT_VERSION_STR  IMGCAT_STR(IMGCAT_VERSION)
#else
#define IMGCAT_VERSION_STR  "v0.0.0-unknown"
#endif
    return IMGCAT_VERSION_STR;
}

}
