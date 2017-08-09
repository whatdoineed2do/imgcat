/*  $Id: ImgExifParser.h,v 1.1 2011/10/30 17:33:53 ray Exp $
 */

#ifndef IMG_EXIF_PARSER_H
#define IMG_EXIF_PARSER_H

struct stat;

#include <stdexcept>
#include "Img.h"

namespace ImgExifParser
{
    const Img  parse(const char* filename_, const struct stat&, const char* thumbpath_)  
	 throw (std::invalid_argument, std::range_error, std::underflow_error, std::overflow_error);
};

#endif
