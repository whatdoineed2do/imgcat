#ident "%W%"

/*  $Id: ImgData.h,v 1.2 2013/10/11 18:13:42 ray Exp $
 */

#ifndef IMG_DATA_H
#define IMG_DATA_H

#pragma ident  "@(#)$Id: ImgData.h,v 1.2 2013/10/11 18:13:42 ray Exp $"

#include <sys/types.h>

#include <iosfwd>
#include <string>
using std::string;


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

    string  filename;
    string  title;

    ImgData::Type  type;
    string  mimetype;
    size_t  size;
    string  thumb;     // path of thumbnail

    string  xy;        // dimensions

    string  moddate;  // may be empty or reset to empty
    string  rating;

    struct MetaImg {
        MetaImg() = default;
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
	MetaImg& operator=(const MetaImg&) = delete;

	string  dpi;
	double  rotate;


	string  camera;
	string  sn;
	string  lens;
	string  focallen;
	string  shuttercnt;

	string  prog;      // PASM
	string  shutter;
	string  aperture;
	string  iso;
	string  wb;
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
	MetaVid& operator-(const MetaVid&) = delete;

        string  container;
	string  model;
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
    void operator=(const ImgData&);

    void  _title()
    {
	string::size_type  p;
	title = ( (p = filename.find_last_of('/')) == string::npos) ? 
	            filename :
		    filename.substr(p+1);
    }
};


std::ostream&  operator<<(std::ostream& os_, const ImgData& obj_);

#endif
