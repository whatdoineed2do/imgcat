#ifndef ICC_PROFILES_H
#define ICC_PROFILES_H

/*  $Id: ICCprofiles.h,v 1.2 2011/10/30 17:42:07 ray Exp $
 *
 *  $Log: ICCprofiles.h,v $
 *  Revision 1.2  2011/10/30 17:42:07  ray
 *  cvs tags
 *
 */

/* od -A d -v -t x1 NKAdobe.icm | awk 'BEGIN { OFS=", 0x" } {$1=""; print }'
 * need to remove the very first ,
 */

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char  uchar_t;

struct ICCprofiles {
    const char*    const  name;
    const uchar_t* const  profile;
    const size_t          length;
};

extern const struct ICCprofiles  theICCprofiles[];
extern const struct ICCprofiles  theSRGBICCprofiles[];


extern const struct ICCprofiles*  theSRGBicc;
extern const struct ICCprofiles*  theARGBicc;
extern const struct ICCprofiles*  theProPhotoRGBicc;

extern const struct ICCprofiles*  theNksRGBicc;
extern const struct ICCprofiles*  theNkaRGBicc;
extern const struct ICCprofiles*  theNkwRGBicc;



#ifdef __cplusplus
}
#endif

#endif
