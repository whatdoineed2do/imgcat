#ident "%W%"

/*  $Id: ImgExifParser.cc,v 1.1 2011/10/30 17:33:52 ray Exp $
 */

#pragma ident  "@(#)$Id: ImgExifParser.cc,v 1.1 2011/10/30 17:33:52 ray Exp $"

#include "ImgExifParser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <strings.h>

#include <exiv2/exiv2.hpp>


const Img  ImgExifParser::parse(const char* filename_, const struct stat& st_, const char* thumbpath_)
    throw (std::invalid_argument, std::range_error, std::underflow_error, std::overflow_error)
{
#if 0
    if (thumbpath_)
    {
	ostringstream  err;

	struct stat  st;
	if (stat(thumbpath_, &st) < 0) {
	    err << "unable to validate thumbpath '" << thumbpath_ << "' - " << strerror(errno);
	    throw invalid_argument(err.str());
	}

	if ( ! (st.st_mode & (S_IFDIR | S_IWUSR)) ) {
	    err << "invalid thumbpath '" << thumbpath_ << "'";
	    throw invalid_argument(err.str());
	}
    }
#endif

    typedef Exiv2::ExifData::const_iterator (*EasyAccessFct)(const Exiv2::ExifData& ed);

    ImgData  data(filename_, st_.st_size);
    string   mftr;
    string   dtorg;
    string   dtorgsub;
    float    orientation = 1;

    static const struct _EasyAccess {
	const char*   desc;
	EasyAccessFct find;
	string*       target;
    } eatags[] = {
	{ "  ISO speed",            Exiv2::isoSpeed,     &data.iso      },
	{ "  Exposure mode",        Exiv2::exposureMode, &data.prog     },
	//{ "  Image quality",        Exiv2::imageQuality, NULL         },
	{ "  White balance",        Exiv2::whiteBalance, &data.wb       },
	{ "  Lens name",            Exiv2::lensName,     &data.lens     },
	//{ "  Metering mode",        Exiv2::meteringMode, NULL         },
	{ "  Camera make",          Exiv2::make,         &mftr          },
	{ "  Camera model",         Exiv2::model,        &data.camera   },   // KEY2
	{ "  Exposure time",        Exiv2::exposureTime, &data.shutter  },
	{ "  FNumber",              Exiv2::fNumber,      &data.aperture },
	{ "  Camera serial number", Exiv2::serialNumber, &data.sn       },   // KEY1
	{ "  Focal length",         Exiv2::focalLength,  &data.focallen },
	{ NULL, NULL, NULL }
    };

    static const struct _MiscTags {
	const char*  desc;
	const char*  tag;
	string*      target;
	long*        tgtlong;
	float*       tgtfloat;
    } misctags[] = {
	{ "  Date Image (mod)",  "Exif.Image.DateTime", &data.moddate, NULL, NULL },           // mod time
#if 0
	{ "  SubSec",            "Exif.Photo.SubSecTime", NULL, NULL, NULL },
	{ "  Date Image Orig",   "Exif.Image.DateTimeOriginal", NULL, NULL, NULL },   // not in CNX2 export'd jpgs
#endif

	{ "  Date Orig",         "Exif.Photo.DateTimeOriginal",   &dtorg, NULL, NULL },   // KEY3
	{ "  SubSec Orig",       "Exif.Photo.SubSecTimeOriginal", &dtorgsub, NULL, NULL }, // KEY4

	{ "  Shutter Count",     "Exif.Nikon3.ShutterCount", &data.shuttercnt, NULL, NULL },

	{ "  resolution",        "Exif.Image.XResolution", &data.dpi, NULL, NULL },
	{ "  orientation",       "Exif.Image.Orientation", NULL, NULL, &orientation },

	{ NULL, NULL, NULL, NULL, NULL }
    };

    static const _MiscTags  xmptags[] = {
	{ "  XMP Rating", "Xmp.xmp.Rating", &data.rating },
	{ NULL, NULL, NULL }
    };

    try
    {
	Exiv2::Image::AutoPtr image;
	try
	{
	    image = Exiv2::ImageFactory::open(filename_);
	
	}
	catch (const Exiv2::AnyError&)
	{
	    ostringstream  err;
	    err << "invalid image file " << filename_;
	    throw std::invalid_argument(err.str());
	}


#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,26,0)
	switch (image->imageType())
	{
	    case Exiv2::ImageType::nef:
	    case Exiv2::ImageType::tiff:
		data.type = ImgData::TIFF;
		break;

	    case Exiv2::ImageType::jpeg:
	    case Exiv2::ImageType::jp2:
	    case Exiv2::ImageType::png:
	    case Exiv2::ImageType::gif:
		data.type = ImgData::IMAGE;
		break;
	}
#else
	if (dynamic_cast<Exiv2::TiffImage*>(image.get()) ) {
	    data.type = ImgData::TIFF;
	}
        else
	{
	    if (dynamic_cast<Exiv2::JpegImage*>(image.get()) ||
	        dynamic_cast<Exiv2::PngImage *>(image.get()) ||
	        dynamic_cast<Exiv2::GifImage *>(image.get())
	       ) 
	    {
		data.type = ImgData::IMAGE;
	    }
	}
#endif


	image->readMetadata();
	const Exiv2::ExifData&  ed = image->exifData();
	if (ed.empty())
	{
	    // need to generate uniq key, will use ino
	    if (thumbpath_) {
		ostringstream  tmp;
		tmp << thumbpath_ << "/" << st_.st_dev << "-" << st_.st_ino;
		data.thumb = tmp.str();
	    }
	    return Img(ImgKey(st_.st_ino, st_.st_mtime), data);
	}


	Exiv2::ExifData::const_iterator  exif;

	const _EasyAccess*  ep = eatags;
	while (ep->desc)
	{
	    if ( (exif = ep->find(ed)) != ed.end())  {
		*ep->target = exif->print(&ed);
	    }
	    ++ep;
	}

	const _MiscTags*  mp = misctags;
	while (mp->desc)
	{
	    if ( (exif = ed.findKey(Exiv2::ExifKey(mp->tag)) ) != ed.end()) {
		if (mp->target)    *mp->target   = exif->print(&ed);
		if (mp->tgtlong)   *mp->tgtlong  = exif->toLong();
		if (mp->tgtfloat)  *mp->tgtfloat = exif->toFloat();
	    }
	    ++mp;
	}

	/* some scans dont have an orig date!
	 */
	if (dtorg.empty()) {
	    char  buf[32];
	    struct tm  tm;
	    localtime_r(&st_.st_mtime, &tm);
	    strftime(buf, 31, "%Y:%m:%d %T", &tm);
	    dtorg = buf;
	}

	if (dtorg == data.moddate) {
	    data.moddate.clear();
	}


	/* determine correct orientation of image, and any rotation requierd
	 */
	switch ((int)orientation) {
	    case 2:  orientation = 0;  break;   // flip horz
	    case 3:  orientation = 180;  break;
	    case 4:  orientation = 0;  break;   // flip vert
	    case 5:  orientation = 0;  break;   // transpose
	    case 6:  orientation = 90;  break;
	    case 7:  orientation = 0;  break;   // traverse
	    case 8:  orientation = -90;  break;

	    case 1:
	    default: orientation = 0;
	}
	data.rotate = orientation;

	const Exiv2::XmpData&  xmp = image->xmpData();
	Exiv2::XmpData::const_iterator  xp;
	mp = xmptags;
	while (mp->desc)
	{
	    if ( (xp = xmp.findKey(Exiv2::XmpKey(mp->tag)) ) != xmp.end()) {
		*mp->target = xp->print(&ed);
	    }
	    ++mp;
	}

	if (thumbpath_)
	{
	    data.thumb = thumbpath_;
	    if (data.thumb[data.thumb.length()-1] != '/') {
		data.thumb.append("/");
	    }

	    ostringstream  id;
	    if (data.sn.empty() && data.shuttercnt.empty()) {
		id << st_.st_dev << "-" << st_.st_ino;
	    }
	    else {
		// this should incl the model too
		id << data.sn << '-' << data.shuttercnt;
	    }
	    data.thumb.append(id.str());
	}

	return Img(ImgKey(mftr.c_str(), data.camera.c_str(), data.sn.c_str(), dtorg.c_str(), dtorgsub.c_str()), data);
    }
    catch (const Exiv2::AnyError& e)
    {
	ostringstream  err;
	err << "unable to extract exif from " << filename_ << " - " << e;
	throw std::underflow_error(err.str());
    }
}
