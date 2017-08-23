#include "ICCprofiles.h"

const struct ICCprofiles  theICCprofiles[] = { 
    { "sRGB", sRGB_IEC61966_2_1, sizeof(sRGB_IEC61966_2_1) },
    { "Adobe98", Adobe98, sizeof(Adobe98) },
    { "ProPhoto", ProPhoto, sizeof(ProPhoto) },


    { "Nikon sRGB", NKsRGB, sizeof(NKsRGB) },
    { "Nikon aRGB", NKAdobe, sizeof(NKAdobe) },
    { "Nikon wide RGB", NKWide, sizeof(NKWide) }
};

const struct ICCprofiles  theSRGBICCprofiles[] = {
    { "Nikon sRGB", NKsRGB, sizeof(NKsRGB) },
    { "sRGB", sRGB_IEC61966_2_1, sizeof(sRGB_IEC61966_2_1) }
};

const struct ICCprofiles*  theSRGBicc        = &theICCprofiles[0];
const struct ICCprofiles*  theARGBicc        = &theICCprofiles[1];
const struct ICCprofiles*  theProPhotoRGBicc = &theICCprofiles[2];

const struct ICCprofiles*  theNksRGBicc = &theICCprofiles[3];
const struct ICCprofiles*  theNkaRGBicc = &theICCprofiles[4];
const struct ICCprofiles*  theNkwRGBicc = &theICCprofiles[5];


