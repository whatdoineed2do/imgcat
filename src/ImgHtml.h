#ifndef IMG_HTML_H
#define IMG_HTML_H

#include "ImgOut.h"

#include <string>


class ImgHtml : public ImgOut
{
   public:
     virtual ~ImgHtml() = default;

     ImgHtml(const ImgHtml&) = delete;
     ImgHtml& operator=(const ImgHtml&) = delete;
     ImgHtml(const ImgHtml&&) = delete;
     ImgHtml& operator=(const ImgHtml&&) = delete;

     std::string  filename() final
     { return std::string("index.html"); }

   protected:
     ImgHtml() = default;
};


struct ImgHtmlClassic : public ImgHtml
{
    static constexpr const char*  ID = "classic";

    ImgHtmlClassic() = default;

    std::string  generate(ImgOut::Payloads&)  override;
};

class ImgHtmlFlexbox : public ImgHtml
{
  public:
    ImgHtmlFlexbox() = default;
    virtual ~ImgHtmlFlexbox() = default;

    std::string  generate(ImgOut::Payloads&)  final;

  protected:
    virtual const char*  _jsblock() = 0;
};

class ImgHtmlFlexboxSlide : public ImgHtmlFlexbox
{
  public:
    static constexpr const char*  ID = "flexbox-slide";

    ImgHtmlFlexboxSlide() = default;
    virtual ~ImgHtmlFlexboxSlide() = default;

  private:
    const char*  _jsblock()  override;
};

class ImgHtmlFlexboxHide : public ImgHtmlFlexbox
{
  public:
    static constexpr const char*  ID = "flexbox-hide";

    ImgHtmlFlexboxHide() = default;
    virtual ~ImgHtmlFlexboxHide() = default;

  private:
    const char*  _jsblock()  override;
};


// justified gallery
class ImgHtmlJG: public ImgHtml
{
  public:
    static constexpr const char*  ID = "justified-gallery";

    ImgHtmlJG() = default;
    virtual ~ImgHtmlJG() = default;

    std::string  generate(ImgOut::Payloads&)  final;

  private:
    static const char*  _css_justified_gallery;
    static const char*  _js_justified_gallery;
    static const char*  _js_jquery;
};


#endif
