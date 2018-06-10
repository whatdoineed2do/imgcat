#ifndef IMG_THUMB_GEN_H
#define IMG_THUMB_GEN_H

#include <sys/time.h>

#include <string>
#include <list>
#include <sstream>

#include <exiv2/exiv2.hpp>
#include <Magick++.h>

#include "ImgIdx.h"
#include "ImgData.h"


inline suseconds_t operator-(const timeval& lhs_, const timeval& rhs_)
{
    const suseconds_t x = lhs_.tv_sec*1000000 + lhs_.tv_usec;
    const suseconds_t y = rhs_.tv_sec*1000000 + rhs_.tv_usec;
    return x - y;
}



class ImgThumbGen
{
  public:
    ImgThumbGen(const ImgIdx::Ent& imgidx_, unsigned thumbsize_)
	: _imgidx(imgidx_), _img(*_imgidx.imgs.begin()), _prevpath(_img.thumb + ".jpg"),
	  thumbsize(thumbsize_)
    { }

    ~ImgThumbGen() = default;

    ImgThumbGen(const ImgThumbGen&) = delete;
    ImgThumbGen& operator=(const ImgThumbGen&) = delete;

    ImgThumbGen(ImgThumbGen&& rhs_)
    	: _imgidx(rhs_._imgidx), _img(rhs_._img), _prevpath(std::move(rhs_._prevpath)),
	  thumbsize(rhs_.thumbsize)
    { }

    ImgThumbGen& operator=(ImgThumbGen&& rhs_) = delete;


    void  generate();


    const ImgIdx::Ent&  idx() const
    {
	return _imgidx;
    }

    const ImgData&  img() const
    {
	return _img;
    }

    const std::string&  prevpath() const
    {
	return _prevpath;
    }


    double  ms() const
    {
	return (double)(_y - _x)/1000000;
    }


    const std::string  error() const
    { return _error.str(); }

    const unsigned  thumbsize;

  private:
    const ImgIdx::Ent&  _imgidx;
    const ImgData&  _img;
    const std::string  _prevpath;


    struct timeval  _x, _y;

    std::ostringstream  _error;

    void  _genthumbnail(const std::string& path_, const std::string& origpath_,
                        Magick::Image& img_,
                        const unsigned sz_, const float rotate_);

    void  _genthumbnail(const std::string& path_, const std::string& origpath_,
                        const void* data_, const size_t datasz_,
                        const unsigned sz_, const float rotate_);

    void  _genthumbnail(const std::string& path_, const std::string& origpath_,
                        const Exiv2::PreviewImage& preview_, const Exiv2::ExifData& exif_,
                        const unsigned sz_, const float rotate_);

    void  _readgenthumbnail(const ImgData& img_, const std::string& prevpath_, const unsigned sz_);

};

using ImgThumbGens = std::list<ImgThumbGen*>;


#endif
