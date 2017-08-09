/* $Id: diptych.cc,v 1.8 2012/04/07 12:40:57 ray Exp $
 *
 * $Log: diptych.cc,v $
 * Revision 1.8  2012/04/07 12:40:57  ray
 * work out -s when not provided
 *
 * Revision 1.7  2012/04/05 19:14:41  ray
 * use resize() for choice of IM filter algorithms
 *
 * Revision 1.6  2012/02/01 12:55:22  ray
 * *** empty log message ***
 *
 * Revision 1.5  2011/12/16 20:44:51  ray
 * use real time on final exif to avoid confusing most exif parsers
 *
 * Revision 1.2  2011/10/30 17:38:55  ray
 * cvs tags
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <limits.h>
#include <getopt.h>
#include <libgen.h>

#include <string>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <list>

using namespace  std;

#include <Magick++.h>

#ifdef GEN_EXIF
#include <exiv2/exiv2.hpp>
#endif

#define DIPTYCH_VERBOSE_LOG(x) if (thegopts.verbose) { cout << x << endl; }

#ifdef DEBUG_LOG
#define DIPTYCH_DEBUG_LOG(x) { cout << "DEBUG:  " << x << endl; }
#else
#define DIPTYCH_DEBUG_LOG(x)
#endif

/*
   g++ -DGEN_EXIF -DNEED_UCHAR_UINT_T diptych.cc \
       $(/usr/local/bin/Magick++-config --cppflags --cxxflags --libs) \
       -lexiv2 -lexpat -lz
 */
#ifdef NEED_UCHAR_UINT_T
typedef unsigned char  uchar_t;
typedef unsigned int   uint_t;
#endif

ostream&  operator<<(ostream& os_, const Magick::Geometry& obj_)
{
    return os_ << (string)obj_;
}

struct _Gopts {
    struct Brdr {
	const char*  colour;
	uint_t  width;
    } border, frame;

    struct Output {
	uint_t  quality;
	const char*  output;
	const char*  size;
    } output;

    const char*  resolution;

    struct {
	void (Magick::Image::*fptr)(const Magick::Geometry&);
	Magick::FilterTypes  filter;
	float  ratio;
    } scale;

    bool  verbose;
} thegopts =
{
    { "white", 10 },
    //{ thegopts.border.colour, 3 },
    { "white", 3 },
    { 100, NULL, NULL },
    "300x300",
    { &Magick::Image::resize, Magick::UndefinedFilter, 1 },
    false
};

namespace diptych
{
bool  operator==(const Magick::Geometry& lhs_, const Magick::Geometry& rhs_)
{
    bool  w = true;
    bool  h = true;

    if (lhs_.width()  && rhs_.width())    w = lhs_.width()  == rhs_.width();
    if (lhs_.height() && rhs_.height())   h = lhs_.height() == rhs_.height();

    return w && h;
}

bool  operator!=(const Magick::Geometry& lhs_, const Magick::Geometry& rhs_)
{ return !diptych::operator==(lhs_, rhs_); }

bool  operator<(const Magick::Geometry& lhs_, const Magick::Geometry& rhs_)
{
    bool  w = true;
    bool  h = true;

    if (lhs_.width()  && rhs_.width())    w = lhs_.width()  < rhs_.width();
    if (lhs_.height() && rhs_.height())   h = lhs_.height() < rhs_.height();

    return w && h;
}

Magick::Geometry&  operator/=(Magick::Geometry& obj_, const float n_)
{
    if (obj_.width()) {
	obj_.width(obj_.width()/n_);
    }

    if (obj_.height()) {
	obj_.height(obj_.height()/n_);
    }

    return obj_;
}
}

void DIPTYCH_SCALE(Magick::Image& img_, const Magick::Geometry& g_, const float ratio_)
{
    if (thegopts.scale.filter != Magick::UndefinedFilter) {
	img_.filterType(thegopts.scale.filter);
    }

    if (ratio_ <= 1) {
	DIPTYCH_DEBUG_LOG(&img_ << "  scaling: direct to=" << g_.height() << "x" << g_.width());
	(img_.*thegopts.scale.fptr)(g_);
	return;
    }


    using namespace diptych;
    Magick::Geometry  g(img_.columns(), img_.rows());

    DIPTYCH_DEBUG_LOG(&img_ << "  scaling: from=" << g.height() << "x" << g.width() << " to=" << g_.height() << "x" << g_.width());

    /* scale the img by steps to avoid posterisation effects */
    while (diptych::operator!=(g, g_) )
    {
	g /= ratio_;
	DIPTYCH_DEBUG_LOG(&img_ << "  scaling: " << g.height() << "x" << g.width());
	if (diptych::operator<(g, g_)) {
	    g = g_;
	    DIPTYCH_DEBUG_LOG(&img_ << "  scaling: " << g.height() << "x" << g.width() << " - final");
	}
	(img_.*thegopts.scale.fptr)(g);
	//img_.resize(g);
    }
}



struct Padding
{
    Padding(const uint_t intnl_, const uint_t extnl_) : intnl(intnl_), extnl(extnl_) { }

    const uint_t  intnl;
    const uint_t  extnl;
};

class Frame
{
  public:
    virtual ~Frame() { }

   virtual Magick::Image  process(const uint_t)  throw (underflow_error, logic_error) = 0;

  protected:
    Frame() { }

  private:
    Frame(const Frame&);
    void operator=(const Frame&);
};

class ImgFrame;

class _ImgFrame
{
  public:
    friend class ImgFrame;

    _ImgFrame(Magick::Image& img_) : _img(img_), _full(true) { }

    _ImgFrame(const char* img_) : _img(img_), _full(false)
    { _img.ping(img_); }

    ~_ImgFrame() { }

    uint_t  rows() const { return _img.rows(); }
    uint_t  cols() const { return _img.columns(); }

    Magick::Image&  _read()
    {
	if (_full) { return _img; }

	_img.read(_img.fileName());
	_full = true;
	return _img;
    }

  private:
    Magick::Image  _img;
    bool  _full;
};


class ImgFrame : public Frame
{
  public:
    typedef list<_ImgFrame*>  Imgs;
    typedef list<Magick::Image>  _MImgs;

    struct Exif
    {
	static const string  TAG_make;
	static const string  TAG_model;
	static const string  TAG_dateorig;
	static const string  TAG_artist;
	static const string  TAG_copyright;
	static const string  TAG_maxaperture;
	static const string  TAG_focallen;

	Exif() { }

	Exif(const Magick::Image& img_)
	{
	    Magick::Image& img = (Magick::Image&)img_;
	    make     = img.attribute(TAG_make);
	    model    = img.attribute(TAG_model);

	    dateorig = img.attribute(TAG_dateorig);
	    artist    = img.attribute(TAG_artist);
	    copyright = img.attribute(TAG_copyright);
	    maxaperture = img.attribute(TAG_maxaperture);
	    focallen  = img.attribute(TAG_focallen);
	}

	Exif(const Exif& rhs_) : make(rhs_.make), model(rhs_.model), dateorig(rhs_.dateorig), artist(rhs_.artist), copyright(rhs_.copyright), maxaperture(rhs_.maxaperture), focallen(rhs_.focallen)
	{ }

	const Exif& operator=(const Exif& rhs_)
	{
	    if (&rhs_ != this) {
		make      = rhs_.make;
		model     = rhs_.model;
		dateorig  = rhs_.dateorig;
		artist    = rhs_.artist;
		copyright = rhs_.copyright;
		maxaperture = rhs_.maxaperture;
		focallen  = rhs_.focallen;
	    }
	    return *this;
	}

	const bool operator==(const Exif& rhs_) const
	{
	    if (&rhs_ == this) {
		return true;
	    }

	    string  a = dateorig;
	    string  b = rhs_.dateorig;
	    string::size_type  p = a.find(" ");
	    if (p != string::npos) {
		a.erase(p);
	    }
	    p = b.find(" ");
	    if (p != string::npos) {
		b.erase(p);
	    }

	    return make == rhs_.make && model == rhs_.model && a == b;
	}

	const bool operator!=(const Exif& rhs_) const
	{ return !operator==(rhs_); }

	operator bool() const 
	{ return make.empty() ? false : true; }


	void  copy(Magick::Image& img_) const
	{
	    if (!make.empty())         img_.attribute(TAG_make,        make);
	    if (!model.empty())        img_.attribute(TAG_model,       model);
	    if (!dateorig.empty())     img_.attribute(TAG_dateorig,    dateorig);
	    if (!artist.empty())       img_.attribute(TAG_artist,      artist);
	    if (!copyright.empty())    img_.attribute(TAG_copyright,   copyright);
	    if (!maxaperture.empty())  img_.attribute(TAG_maxaperture, maxaperture);
	    if (!focallen.empty())     img_.attribute(TAG_focallen,    focallen);
	}


	bool  clean(const Exif& rhs_)
	{
	    if (make != rhs_.make) {
		return false;
	    }

	    if (model       != rhs_.model)        model.clear();
	    if (dateorig    != rhs_.dateorig)     dateorig.clear();
	    if (artist      != rhs_.artist)       artist.clear();
	    if (copyright   != rhs_.copyright)    copyright.clear();
	    if (maxaperture != rhs_.maxaperture)  maxaperture.clear();
	    if (focallen    != rhs_.focallen)     focallen.clear();

	    return true;
	}


	string  make;
	string  model;
	string  dateorig;

	string  artist;
	string  copyright;
	string  maxaperture;
	string  focallen;
    };

    virtual ~ImgFrame()
    {
	for (ImgFrame::Imgs::iterator i=_imgs.begin(); i!=_imgs.end(); ++i) {
	    delete *i;
	}
	delete _exif;
    }

    virtual void  push_back(Magick::Image img_)
    {
	_imgs.push_back(new _ImgFrame(img_));
	_ttly += _imgs.back()->rows();
	_ttlx += _imgs.back()->cols();

	DIPTYCH_DEBUG_LOG(this << " : adding M:  ttlx=" << _ttlx << " ttly=" << _ttly << ", x/y=" << _imgs.back()->cols() << "x" << _imgs.back()->rows());

	_tracksmallest(*_imgs.back());
    }

    virtual void  push_back(const char* img_)
    {
	_imgs.push_back(new _ImgFrame(img_));
	_ttly += _imgs.back()->rows();

	DIPTYCH_DEBUG_LOG(this << " : adding c*:  ttly=" << _ttly << ", x/y cols/rows=" << _imgs.back()->cols() << "x" << _imgs.back()->rows() << "  " << img_);

	_tracksmallest(*_imgs.back());
    }

    const _ImgFrame&  back() const
    { return *_imgs.back(); }

    Magick::Image  process(const uint_t)  throw (underflow_error, logic_error);

    /* consolidated exif across all the push_back'd img frames
     */
    const ImgFrame::Exif&  exif() const
    { 
	static ImgFrame::Exif  tmp;
	return _exif ? *_exif : tmp;
    }

    const uint_t  size() const
    { return _imgs.size(); }

    const uint_t  ttly()
    {
	uint_t  y = 0;
	for (ImgFrame::Imgs::const_iterator i=_imgs.begin(); i!=_imgs.end(); ++i)
	{
	    const _ImgFrame&  img = **i;

	    // figure out the scaling ratio so that all X are the same -
	    // affects the Y

	    const double  sr = (double)_smallest->cols() / (double)img.cols();
	    y += img.rows() * sr;

	    DIPTYCH_DEBUG_LOG("IF=" << this << " : scaled Y: smallest x=" << _smallest->cols() << ", curr x=" << img.cols() << " ratio=" << sr << "  ttly=" << y << "  (y scaled from=" << img.rows() << " to=" << img.rows()*sr << ")");
	}
	DIPTYCH_DEBUG_LOG("IF=" << this << ": final ttly=" << y);
	_ttly = y;

	return _ttly;
    }


  protected:
    ImgFrame(Padding&  padding_) : _exif(NULL), _ttlx(0), _ttly(0), _padding(padding_), _smallest(NULL) { }

    ImgFrame::Exif*  _exif;

    ImgFrame::Imgs  _imgs;

    virtual Magick::Image  _process(const uint_t  trgt_) = 0;

    bool  _cmpExif(const Magick::Image& img_)
    {
	bool  b = false;

	const ImgFrame::Exif  e(img_);
	if (_exif == NULL) {
	    _exif = new ImgFrame::Exif(e);
	}
	else
	{
	    if (*_exif == e) {
	    }
	    else
	    {
		DIPTYCH_DEBUG_LOG("   " << this << " exif mismatch - cleaning: orig=" << *_exif << "new=" << e);

		if (_exif->clean(e)) {
		}
		else {
		    b = true;
		    delete _exif;
		    _exif = NULL;
		}
	    }
	}
	return b;
    }

    /*mutable*/ Padding&  _padding;

    uint_t  _ttlx;
    uint_t  _ttly;

    const _ImgFrame*  _smallest;  // represents the smallest img along X

  private:
    void  _tracksmallest(const _ImgFrame& img_)
    {
	/* keep track of the min x (width) and the relevant scale ratio across
	 * all imgs
	 */
	if (_smallest == NULL) {
	    _smallest = &img_;
	}
	else
	{
	    if (img_.cols() < _smallest->cols()) {
		_smallest = &img_;
	    }
	}
    }
};

const string  ImgFrame::Exif::TAG_make      = "exif:Make";
const string  ImgFrame::Exif::TAG_model     = "exif:Model";
const string  ImgFrame::Exif::TAG_dateorig  = "exif:DateTimeOriginal";
const string  ImgFrame::Exif::TAG_artist    = "exif:Artist";
const string  ImgFrame::Exif::TAG_copyright = "exif:Copyright";
const string  ImgFrame::Exif::TAG_maxaperture = "exif:MaxApertureValue";
const string  ImgFrame::Exif::TAG_focallen  = "exif:FocalLength";

ostream&  operator<<(ostream& os_, const ImgFrame::Exif& obj_)
{
    return os_ << "make=" << obj_.make << " model=" << obj_.model << " date=" << obj_.dateorig << " focallen=" << obj_.focallen << " max f/=" << obj_.maxaperture;
}

Magick::Image  ImgFrame::process(const uint_t  trgt_)  throw (underflow_error, logic_error)
{
    if (_imgs.empty()) {
	throw underflow_error("ImgFrame: no internal imgs");
    }
    /* everything is now same width or height so need to create the final
     * img
     */
    Magick::Image  img = _process(trgt_);

#ifdef GEN_EXIF
    if (exif())
    {
	try
	{
	    // and attach the exif
	    Exiv2::Blob      evraw;
	    Exiv2::ExifData  evexif;

	    evexif["Exif.Image.Make"]     = exif().make;
	    evexif["Exif.Image.Model"]    = exif().model;
	    evexif["Exif.Photo.DateTimeOriginal"] = exif().dateorig;
	    evexif["Exif.Image.Artist"]   = exif().artist;
	    evexif["Exif.Image.Copyright"] = exif().copyright;
	    evexif["Exif.Photo.MaxApertureValue"] = exif().maxaperture;
	    evexif["Exif.Photo.FocalLength"] = exif().focallen;

	    Exiv2::ExifParser::encode(evraw, Exiv2::littleEndian, evexif);
	    uchar_t*  ebuf = new uchar_t[6+evraw.size()];
	    ebuf[0] = 'E';
	    ebuf[1] = 'x';
	    ebuf[2] = 'i';
	    ebuf[3] = 'f';
	    ebuf[4] = 0;
	    ebuf[5] = 0;
	    memcpy(ebuf+6, &evraw[0], evraw.size());

	    img.exifProfile(Magick::Blob(ebuf, 6+evraw.size()));
	    delete [] ebuf;

	    DIPTYCH_VERBOSE_LOG("encoded exif=" << ImgFrame::Exif(img));
	}
	catch (const exception& ex)
	{
	    cerr << "failed to attached generated exif - " << ex.what() << endl;
	}
    }
#endif
    return img;
}


class VImgFrame : public ImgFrame
{
  public:
    VImgFrame(Padding& p_) : ImgFrame(p_)
    { }

  private:
    Magick::Image  _process(const uint_t  trgt_)
    {
	ImgFrame::_MImgs  imgs;

	/* scale to the target height, taking care that resulting scaled 
	 * internal
	 * images reflect the req'd padding
	 */
	const uint_t  ttlpad = _padding.intnl * (_imgs.size()-1);
	const uint_t  adjust = _imgs.size() == 1 ? 0 : ceil(ttlpad/_imgs.size());
	const double  sr     = _imgs.size() == 1 ? 1 : trgt_/(double)_ttly;

	uint_t  remain = trgt_;

	DIPTYCH_DEBUG_LOG("VF=" << this << " : target=" << trgt_ << " padding=" << _padding.intnl << " imgs=" << _imgs.size() << " ttl pad=" << ttlpad << " adjust=" << adjust << " scale ratio=" << sr);

	const double  smallestx = _smallest->cols();
	bool  skip = false;
	uint_t  j = 0;
	for (ImgFrame::Imgs::iterator i=_imgs.begin(); i!=_imgs.end(); ++i)
	{
	    Magick::Image&  img = (*i)->_read();

	    uint_t  trgt = 0;
	    if (++j == _imgs.size())
	    {
		/* dealing with the last frame, need to make sure that we have
		 * filled met trgt_ size
		 */
		trgt = remain - adjust;
	    }
	    else
	    {
		const double  wsr = smallestx / (double)img.columns();
		trgt = (img.rows() * wsr * sr) - adjust;
	    }
	    remain -= trgt+adjust;

	    DIPTYCH_DEBUG_LOG("VF=" << this << " " << j << '/' << _imgs.size() << " scale from=" << img.columns() << "x" << img.rows() << " trgt y=" << trgt << "  remain y=" << remain << "/" << trgt_);

	    DIPTYCH_SCALE(img, Magick::Geometry(0, trgt), thegopts.scale.ratio);

	    DIPTYCH_DEBUG_LOG("VF=" << this << " " << j << '/' << _imgs.size() << "         to=" << img.columns() << "x" << img.rows() << "  exif=" << ImgFrame::Exif(img));
	    imgs.push_back(img);


	    if (skip) {
		continue;
	    }
	    skip = _cmpExif(img);
	}

	/* never has border but internal padding
	 * stack imgs top-bottom
	 */

	Magick::Image  dest(Magick::Geometry(imgs.front().columns(), trgt_), thegopts.border.colour);

	DIPTYCH_VERBOSE_LOG("vert frame dest cols=" << dest.columns() << " rows=" << dest.rows() << " seperator=" << _padding.intnl << " colour=" << thegopts.border.colour);

	uint_t  y = 0;
	for (ImgFrame::_MImgs::const_iterator i=imgs.begin(); i!=imgs.end(); ++i)
	{
	    const Magick::Image&  img = *i;
	    dest.composite(img, 0, y);

	    DIPTYCH_VERBOSE_LOG("  y=" << setw(5) << y << " input cols=" << img.columns() << " rows=" << img.rows() << "  (" << i->fileName() << ")  { " << ImgFrame::Exif(img) << " }");

	    DIPTYCH_DEBUG_LOG("VF=" << this << " stacking y pos=" << y << " img rows=" << img.rows());

	    y += img.rows() + _padding.intnl;
	}

	return dest;
    }
};

class HImgFrame : public ImgFrame
{
  public:
    HImgFrame(Padding& p_) : ImgFrame(p_)
    { }

  private:
    Magick::Image  _process(const uint_t  trgt_)
    {
	ImgFrame::_MImgs  imgs;

	bool  skip = false;
	for (ImgFrame::Imgs::iterator i=_imgs.begin(); i!=_imgs.end(); ++i)
	{
	    Magick::Image&  img = (*i)->_read();
	    imgs.push_back(img);

	    if (skip) {
		continue;
	    }
	    skip = _cmpExif(img);
	}


	/* stack left to right
	 */

	uint_t  w = 0;
	for (ImgFrame::_MImgs::const_iterator i=imgs.begin(); i!=imgs.end(); ++i) {
	    if (i != imgs.begin()) {
		w += _padding.intnl;
	    }
	    w += i->columns();
	}


	Magick::Image  dest(Magick::Geometry(w+2*_padding.extnl, imgs.front().rows()+2*_padding.extnl), thegopts.frame.colour);
	DIPTYCH_DEBUG_LOG("HF=" << this << " target=" << dest.columns() << "x" << dest.rows() << " (w/o border=" << w << 'x' << imgs.front().rows() << ")");

	dest.resolutionUnits(Magick::PixelsPerInchResolution);
	dest.density(thegopts.resolution);

	DIPTYCH_VERBOSE_LOG("horz frame dest cols=" << dest.columns() << " rows=" << dest.rows() << " border=" << _padding.extnl << " colour=" << thegopts.frame.colour);

	uint_t  x = 0;
	uint_t  y = 0;

	/* requested an external border to the entire final composite?
	 */
	if (_padding.extnl) {
	    x = _padding.extnl;
	    y = _padding.extnl;
	}

	for (ImgFrame::_MImgs::const_iterator i=imgs.begin(); i!=imgs.end(); ++i) {
	    const Magick::Image&  img = *i;
	    dest.composite(img, x, y);
	    DIPTYCH_VERBOSE_LOG("  x=" << setw(5) << x << " input cols=" << img.columns() << " rows=" << img.rows());
	    x += img.columns() + _padding.intnl;
	}

	return dest;
    }
};

typedef unsigned short  ushort_t;


int main(int argc, char* const argv[])
{
    const char*  argv0 = basename(argv[0]);

    list<ushort_t>  verts;
    uint_t  reqfiles = 1;

    int  c;
    while ( (c = getopt(argc, argv, "b:B:c:C:s:O:ho:q:vr:R:S:f:")) != EOF) {
	switch (c)
	{
	    case 'b':
	    {
		thegopts.border.width = (uint_t)atoi(optarg);
	    } break;

	    case 'B':
	    {
		thegopts.frame.width = (uint_t)atoi(optarg);
	    } break;

	    case 'c':
	    {
	        try
		{
		    Magick::Color  c(optarg);
		    thegopts.border.colour = optarg;
		}
		catch (const exception& ex)
		{ }
	    } break;

	    case 'C':
	    {
	        try
		{
		    Magick::Color  c(optarg);
		    thegopts.frame.colour = optarg;
		}
		catch (const exception& ex)
		{ }
	    } break;

	    case 's':
	    {
		char*  q = optarg;
		char*  p = q;

		reqfiles = 0;

		while ( (p = strchr(q, ':')) )
		{
		    *p = (char)NULL;
		    verts.push_back((ushort_t)atoi(q));
		    reqfiles += verts.back();

		    q = p+1;
		}
		verts.push_back((ushort_t)atoi(q));
		reqfiles += verts.back();
	    } break;

	    case 'q':
	    {
		thegopts.output.quality = (ushort_t)atoi(optarg);
		if (thegopts.output.quality > 100) {
		    thegopts.output.quality = 100;
		}
	    } break;

	    case 'r':
	    {
		try {
		    const Magick::Geometry  g(optarg);
		    thegopts.resolution = optarg;
		}
		catch (...)
		{ }
	    } break;

	    case 'o':
	    {
	        thegopts.output.output = optarg;
	    } break;

	    case 'O':
	    {
		try {
		    const Magick::Geometry  g(optarg);
		    static string  s = g;
		    thegopts.output.size = s.c_str();
		}
		catch (...)
		{ }
	    } break;

	    case 'R':
	    {
		thegopts.scale.fptr = *optarg == 'p' ? 
			&Magick::Image::sample :
			&Magick::Image::scale;
	    } break;

	    case 'f':
	    {
		thegopts.scale.fptr = &Magick::Image::resize;
		struct IMfltrs {
		    const char*          name;
		    Magick::FilterTypes  fltr;
		} imfltrs[] = 
		{
		    "Point", Magick::PointFilter,
		    "Box", Magick::BoxFilter,
		    "Triangle", Magick::TriangleFilter,
		    "Hermite", Magick::HermiteFilter,
		    "Hanning", Magick::HanningFilter,
		    "Hamming", Magick::HammingFilter,
		    "Blackman", Magick::BlackmanFilter,
		    "Gaussian", Magick::GaussianFilter,
		    "Quadratic", Magick::QuadraticFilter,
		    "Cubic", Magick::CubicFilter,
		    "Catrom", Magick::CatromFilter,
		    "Mitchell", Magick::MitchellFilter,
		    "Jinc", Magick::JincFilter,
		    "Sinc", Magick::SincFilter,
		  //"SincFast", Magick::SincFastFilter,
		  //"Kaiser", Magick::KaiserFilter,
		  //"Welsh", Magick::WelshFilter,
		  //"Parzen", Magick::ParzenFilter,
		  //"Bohman", Magick::BohmanFilter,
		  //"Bartlett", Magick::BartlettFilter,
		  //"Lagrange", Magick::LagrangeFilter,
		    "Lanczos", Magick::LanczosFilter,
		  //"LanczosSharp", Magick::LanczosSharpFilter,
		  //"Lanczos2", Magick::Lanczos2Filter,
		  //"Lanczos2Sharp", Magick::Lanczos2SharpFilter,
		  //"Robidoux", Magick::RobidouxFilter,
		    NULL, Magick::UndefinedFilter
		};
		const IMfltrs*  p = imfltrs;
		while (p->name != NULL) {
		    if (strcasecmp(p->name, optarg) == 0) {
			thegopts.scale.filter = p->fltr;
			break;
		    }
		    ++p;
		}

		if (p->name == NULL) {
		    cerr << "unknown IM resize filter '" << optarg << "' valid options: ";
		    p = imfltrs;
		    while (p->name != NULL) {
			cerr << p->name << " ";
			++p;
		    }
		    cerr << endl;
		    goto usage;
		}
	    } break;


	    case 'S':
	    {
		thegopts.scale.ratio = atof(optarg);
	    } break;

	    case 'v':
	    {
	        thegopts.verbose = true;
	    } break;

usage:
	    case 'h':
	    default:
	        cout << argv0 << "  version $Id: diptych.cc,v 1.8 2012/04/07 12:40:57 ray Exp $  (with"
#ifdef GEN_EXIF
#else
		     << "out"
#endif
		     << " exif support)\n"
		     << "usage:  " << argv0 << " [ -b <border width> -c <border colour> ] [ -B <external border width> -C <external border colour> ] [ -q <output quality%> ] [ -r <resolution> ] -s <components, ie 2:1> -o <output> [ -O <output geometry> ]  [-f <resize filter=lanczos|...]  <files>" << endl
		     << "        -B  = " << thegopts.frame.width << "pxls -C = " << thegopts.frame.colour << endl
		     << "        -b  = " << thegopts.border.width << "pxls -c = " << thegopts.border.colour << endl
		     << "        -q  = " << thegopts.output.quality << endl
		     << "        -r  = " << thegopts.resolution << endl;

		return 1;
	}
    }

    if (optind == argc) {
	cerr << argv0 << ": no files" << endl;
	goto usage;
    }

    if (thegopts.output.output == NULL) {
	goto usage;
    }

    if (verts.empty())
    {
	// no -s given!  figure out what we should do lets 1:1:1...
	reqfiles = argc - optind;
	int  o = optind;
	while (o < argc) {
	    verts.push_back(1);
	    ++o;
	}
    }

    if (reqfiles != argc - optind) {
	cerr << argv0 << ": insufficient files, req=" << reqfiles << " provided=" << argc-optind << endl;
	goto usage;
    }


    list<VImgFrame*>  verticals;
    list<Padding>  paddings;

    /* this is the target height of the final HImgFrame, based on the smallest
     * ttl Y of all the vertical frames
     */
    uint_t  trgty = 0;

    for (list<ushort_t>::iterator i = verts.begin(); i!=verts.end(); ++i)
    {
	// internal (ie verticals) don't have an outter border
	paddings.push_back(Padding(thegopts.border.width, 0));
	verticals.push_back(new VImgFrame(paddings.back()));

	ushort_t  N = *i;
	while (N-- > 0 && optind < argc)
	{
	    const char*  file = argv[optind++];

	    if (access(file, R_OK) != 0) {
		cerr << argv0 << ": " << file << " - " << strerror(errno) << endl;
		return 1;
	    }

	    try
	    {
		verticals.back()->push_back(file);
	    }
	    catch (const exception& ex)
	    {
		cerr << argv0 << ": unable to read file for composing verticals: " << file << " - " << ex.what() << endl;
		return 1;
	    }
	}

	const uint_t  y = verticals.back()->ttly();
	if (trgty == 0 || y < trgty) {
	    DIPTYCH_DEBUG_LOG("re-eval for Y final target from=" << trgty << " to=" << y << "  based on " << verticals.back());
	    // doing this can scale up imgs!!
	    //trgty = y + (verticals.back()->size()-1) * thegopts.border.width;
	    trgty = y;
	}
    }

    int  retcode = 0;
    try
    {
	Padding  hpad(thegopts.border.width, thegopts.frame.width);
	HImgFrame  horizontals(hpad);
	DIPTYCH_DEBUG_LOG("hframe=" << &horizontals << " - generating verticals/scalings");

	for (list<VImgFrame*>::iterator i=verticals.begin(); i!=verticals.end(); ++i)
	{
	    try
	    {
		/* ensure that all vertical frames are the same (trgty) height
		 */
		horizontals.push_back( (*i)->process(trgty) );
	    }
	    catch (const exception& ex)
	    {
		cerr << argv0 << ": failed to generate vertical component - " << ex.what() << endl;
		throw;
	    }
	}
	Magick::Image  final = horizontals.process(trgty);

	if (thegopts.output.size) {
	    const Magick::Geometry  a = final.size();
	    const Magick::Geometry  b = thegopts.output.size;

	    const ImgFrame::Exif  e(final);
	    DIPTYCH_SCALE(final, b, thegopts.scale.ratio);
	    e.copy(final);
	}
	final.quality(thegopts.output.quality);

	try
	{
	    final.write(thegopts.output.output);
	}
	catch (const exception& ex)
	{
	    ostringstream  out;
	    out << "/tmp/" << argv0 << "." << time(NULL) << "." << getpid() << ".jpg";
	    cerr << argv0 << ": failed to write final image - " << ex.what() << ", will use file=" << out.str() << endl;
	    final.write(out.str());

	    retcode = 1;
	}
    }
    catch (const exception& ex)
    {
	cerr << argv0 << ": failed to generate final image - " << ex.what() << endl;
	retcode = 1;
    }

    for (list<VImgFrame*>::iterator i=verticals.begin(); i!=verticals.end(); ++i) {
	delete *i;
    }

    return retcode;
}
