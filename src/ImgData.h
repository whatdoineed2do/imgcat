#ident "%W%"

/*  $Id: ImgData.h,v 1.2 2013/10/11 18:13:42 ray Exp $
 */

#ifndef IMG_DATA_H
#define IMG_DATA_H

#pragma ident  "@(#)$Id: ImgData.h,v 1.2 2013/10/11 18:13:42 ray Exp $"

#include <sys/types.h>

#include <iosfwd>
#include <string>


class ImgData
{
  public:
    enum Type { UNKNOWN, IMAGE, EMBD_PREVIEW, VIDEO };

    ImgData(const char* filename_, const size_t sz_) : filename(filename_), size(sz_), type(ImgData::UNKNOWN)
    { _title(); }

    ImgData(const ImgData&  rhs_)
	: filename(rhs_.filename), title(rhs_.title), type(rhs_.type), mimetype(rhs_.mimetype), size(rhs_.size), thumb(rhs_.thumb), 
	  xy(rhs_.xy), moddate(rhs_.moddate), rating(rhs_.rating),
	  metaimg(rhs_.metaimg), metavid(rhs_.metavid)
    { }

    ImgData(const ImgData&&  rhs_)
	: filename(std::move(rhs_.filename)), title(std::move(rhs_.title)), type(rhs_.type), mimetype(std::move(rhs_.mimetype)), size(rhs_.size), thumb(std::move(rhs_.thumb)), 
	  xy(std::move(rhs_.xy)), moddate(std::move(rhs_.moddate)), rating(std::move(rhs_.rating)),
	  metaimg(std::move(rhs_.metaimg)), metavid(std::move(rhs_.metavid))
    { }

    void operator=(const ImgData&)  = delete;
    void operator=(const ImgData&&) = delete;

    std::string  filename;
    std::string  title;

    ImgData::Type  type;
    std::string  mimetype;
    size_t  size;
    std::string  thumb;     // path of thumbnail

    std::string  xy;        // dimensions

    std::string  moddate;  // may be empty or reset to empty
    std::string  rating;

    struct MetaImg {
        MetaImg() : rotate(0) { }

	MetaImg(const MetaImg& rhs_) :
	    dpi(rhs_.dpi),
	    rotate(rhs_.rotate),
	    camera(rhs_.camera),
	    sn(rhs_.sn),
	    lens(rhs_.lens),
	    focallen(rhs_.focallen),
	    shuttercnt(rhs_.shuttercnt),
	    prog(rhs_.prog),
	    shutter(rhs_.shutter),
	    aperture(rhs_.aperture),
	    iso(rhs_.iso),
	    wb(rhs_.wb)
	{ }

	MetaImg(const MetaImg&& rhs_) :
	    dpi(std::move(rhs_.dpi)),
	    rotate(rhs_.rotate),
	    camera(std::move(rhs_.camera)),
	    sn(std::move(rhs_.sn)),
	    lens(std::move(rhs_.lens)),
	    focallen(std::move(rhs_.focallen)),
	    shuttercnt(std::move(rhs_.shuttercnt)),
	    prog(std::move(rhs_.prog)),
	    shutter(std::move(rhs_.shutter)),
	    aperture(std::move(rhs_.aperture)),
	    iso(std::move(rhs_.iso)),
	    wb(std::move(rhs_.wb))
	{ }
	MetaImg& operator=(const MetaImg&)  = delete;
	MetaImg& operator=(const MetaImg&&) = delete;

	std::string  dpi;
	double  rotate;


	std::string  camera;
	std::string  sn;
	std::string  lens;
	std::string  focallen;
	std::string  shuttercnt;

	std::string  prog;      // PASM
	std::string  shutter;
	std::string  aperture;
	std::string  iso;
	std::string  wb;
    };
    ImgData::MetaImg  metaimg;

    struct MetaVid {
	MetaVid() : duration(0), framerate(0) { }

	MetaVid(const MetaVid& rhs_) :
            container(rhs_.container),
	    model(rhs_.model),
	    duration(rhs_.duration),
	    framerate(rhs_.framerate)
	{}
	MetaVid(const MetaVid&& rhs_) :
            container(std::move(rhs_.container)),
	    model(std::move(rhs_.model)),
	    duration(rhs_.duration),
	    framerate(rhs_.framerate)
	{}
	MetaVid& operator=(const MetaVid&)  = delete;
	MetaVid& operator=(const MetaVid&&) = delete;

	std::string  container;
	std::string  model;
	time_t  duration;
	float   framerate;
    };
    ImgData::MetaVid  metavid;

    bool  operator==(const ImgData& rhs_) const
    {
	return 
	       metaimg.camera   == rhs_.metaimg.camera &&
	       metaimg.sn       == rhs_.metaimg.sn &&
	       metaimg.lens     == rhs_.metaimg.lens;
    }

    bool  operator!=(const ImgData& rhs_) const
    { return !(*this == rhs_); }

    bool  operator<(const ImgData& rhs_) const
    {
	bool  b = true;
	if (moddate.empty())
	{
	    if (!rhs_.moddate.empty()) {
		b = false;
	    }
	}
	else 
	{
	    if (!rhs_.moddate.empty()) {
		b = moddate < rhs_.moddate;
	    }
	}

	return *this == rhs_ && type > rhs_.type && b;
    }

  private:
    void  _title()
    {
	std::string::size_type  p;
	title = ( (p = filename.find_last_of('/')) == std::string::npos) ? 
	            filename :
		    filename.substr(p+1);
    }
};


std::ostream&  operator<<(std::ostream& os_, const ImgData& obj_);

#endif
