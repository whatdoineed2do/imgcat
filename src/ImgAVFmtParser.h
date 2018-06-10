#ifndef IMG_AVFMT_PARSER_H
#define IMG_AVFMT_PARSER_H

#include "ImgMetaParser.h"
#include <string>


class ImgAVFmtParser : public ImgMetaParser
{
  public:
    ImgAVFmtParser();

  private:
    const Img  _parse(const char* filename_, const struct stat&, const char* thumbpath_)  const
	 throw (std::invalid_argument, std::range_error, std::underflow_error, std::overflow_error) override final;

    mutable char  _averr[256];
    mutable std::string  _tmp;

    static bool  _initd;
};

#endif
