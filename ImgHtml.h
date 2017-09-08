#ifndef IMG_HTML_H
#define IMG_HTML_H

#include <memory>

#include "ImgIdx.h"
#include "ImgThumbGen.h"


class ImgHtml
{
   public:
     virtual ~ImgHtml() = default;

     ImgHtml(const ImgHtml&) = delete;
     ImgHtml& operator=(const ImgHtml&) = delete;
     ImgHtml(const ImgHtml&&) = delete;
     ImgHtml& operator=(const ImgHtml&&) = delete;


     virtual std::string  generate(ImgIdxs&, const ImgThumbGens&) = 0;

     //static std::unique_ptr<ImgHtml>  create(const char* type_)  throw (std::range_error);
     static ImgHtml*  create(const char* type_)  throw (std::range_error);

   protected:
     ImgHtml() = default;
};


struct ImgHtmlClassic : public ImgHtml
{
    static constexpr const char*  ID = "classic";

    ImgHtmlClassic() = default;

    std::string  generate(ImgIdxs&, const ImgThumbGens&)  override;
};

struct ImgHtmlFlexbox : public ImgHtml
{
    static constexpr const char*  ID = "flexbox";

    ImgHtmlFlexbox() = default;

    std::string  generate(ImgIdxs&, const ImgThumbGens&)  override;
};

#endif
