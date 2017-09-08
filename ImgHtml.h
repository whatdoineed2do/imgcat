#ifndef IMG_HTML_H
#define IMG_HTML_H

#include <memory>
#include <list>

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

     struct Payload {
         Payload(ImgIdx& idx_) : idx(idx_) { }
         Payload(Payload&& rhs_) : idx(rhs_.idx), thumbs(std::move(rhs_.thumbs)) { }

         ImgIdx& idx;
         ImgThumbGens thumbs;  // dont own 
     };
     typedef std::list<ImgHtml::Payload>  Payloads;


     virtual std::string  generate(Payloads&) = 0;

     //static std::unique_ptr<ImgHtml>  create(const char* type_)  throw (std::range_error);
     static ImgHtml*  create(const char* type_)  throw (std::range_error);

   protected:
     ImgHtml() = default;
};


struct ImgHtmlClassic : public ImgHtml
{
    static constexpr const char*  ID = "classic";

    ImgHtmlClassic() = default;

    std::string  generate(ImgHtml::Payloads&)  override;
};

struct ImgHtmlFlexbox : public ImgHtml
{
    static constexpr const char*  ID = "flexbox";

    ImgHtmlFlexbox() = default;

    std::string  generate(ImgHtml::Payloads&)  override;
};

#endif
