#ifndef IMG_META_PARSER_H
#define IMG_META_PARSER_H

struct stat;

#include <stdexcept>
#include "Img.h"

struct ImgMetaParser
{
  public:
    ImgMetaParser() = default;
    virtual ~ImgMetaParser() = default;

    const Img  parse(const char* filename_, const struct stat&, const char* thumbpath_)  const
	 throw (std::invalid_argument, std::range_error, std::underflow_error, std::overflow_error);
  protected:
    virtual const Img  _parse(const char* filename_, const struct stat&, const char* thumbpath_)  const
	 throw (std::invalid_argument, std::range_error, std::underflow_error, std::overflow_error) = 0;
};

#endif
