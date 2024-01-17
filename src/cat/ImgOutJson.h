#ifndef IMG_OUT_JSON_H
#define IMG_OUT_JSON_H

#include "ImgOut.h"


class ImgOutJson: public ImgOut
{
  public:
    static constexpr const char*  ID = "json";

    ImgOutJson() = default;
    virtual ~ImgOutJson() = default;

    std::string  filename() final;
    std::string  generate(ImgOut::Payloads&)  final;

  private:
};


class ImgOutJsonJS: public ImgOut
{
  public:
    static constexpr const char*  ID = "jsonjs";

    ImgOutJsonJS() = default;
    virtual ~ImgOutJsonJS() = default;

    std::string  filename() final;
    std::string  generate(ImgOut::Payloads&)  final;

  private:
    ImgOutJson  _impl;
};

#endif
