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

    const Img  parse(const char* filename_, const struct stat&, const char* thumbpath_)  const;

  protected:
    virtual const Img  _parse(const char* filename_, const struct stat&, const char* thumbpath_)  const = 0;
};

#endif
