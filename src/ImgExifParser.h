/*  $Id: ImgExifParser.h,v 1.1 2011/10/30 17:33:53 ray Exp $
 */

#ifndef IMG_EXIF_PARSER_H
#define IMG_EXIF_PARSER_H

#include "ImgMetaParser.h"

class ImgExifParser : public ImgMetaParser
{
  public:
    ImgExifParser() = default;

  private:
    const Img  _parse(const char* filename_, const struct stat&, const char* thumbpath_)  const
	 throw (std::invalid_argument, std::range_error, std::underflow_error, std::overflow_error)  override final; 
};

#endif
