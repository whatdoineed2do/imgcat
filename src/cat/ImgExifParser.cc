#ident "%W%"

/*  $Id: ImgExifParser.cc,v 1.1 2011/10/30 17:33:52 ray Exp $
 */

#pragma ident  "@(#)$Id: ImgExifParser.cc,v 1.1 2011/10/30 17:33:52 ray Exp $"

#include "ImgExifParser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

#include <cmath>
#include <sstream>

#include <exiv2/exiv2.hpp>


const Img  ImgExifParser::_parse(const char* filename_, const struct stat& st_, const char* thumbpath_) const
{
    typedef Exiv2::ExifData::const_iterator (*EasyAccessFct)(const Exiv2::ExifData& ed);

    ImgData  data(filename_, st_.st_size);
    std::string   mftr;
    std::string   dtorg;
    std::string   dtorgsub;
    std::string   lens;
    float    orientation = 1;

    static const struct _EasyAccess {
	const char*   desc;
	EasyAccessFct find;
	std::string*       target;
    } eatags[] = {
	{ "  ISO speed",            Exiv2::isoSpeed,     &data.metaimg.iso      },
	{ "  Exposure mode",        Exiv2::exposureMode, &data.metaimg.prog     },
	//{ "  Image quality",        Exiv2::imageQuality, NULL         },
	{ "  White balance",        Exiv2::whiteBalance, &data.metaimg.wb       },
	{ "  Lens name",            Exiv2::lensName,     &data.metaimg.lens     },
	//{ "  Metering mode",        Exiv2::meteringMode, NULL         },
	{ "  Camera make",          Exiv2::make,         &mftr          },
	{ "  Camera model",         Exiv2::model,        &data.metaimg.camera   },   // KEY2
	{ "  Exposure time",        Exiv2::exposureTime, &data.metaimg.shutter  },
	{ "  FNumber",              Exiv2::fNumber,      &data.metaimg.aperture },
	{ "  Camera serial number", Exiv2::serialNumber, &data.metaimg.sn       },   // KEY1
	{ "  Focal length",         Exiv2::focalLength,  &data.metaimg.focallen },
	{ NULL, NULL, NULL }
    };

    static const struct _MiscTags {
	const char*  desc;
	const char*  tag;
	std::string*      target;
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

	{ "  Shutter Count",     "Exif.Nikon3.ShutterCount", &data.metaimg.shuttercnt, NULL, NULL },

	{ "  resolution",        "Exif.Image.XResolution", &data.metaimg.dpi, NULL, NULL },
	{ "  orientation",       "Exif.Image.Orientation", NULL, NULL, &orientation },

	{ NULL, NULL, NULL, NULL, NULL }
    };

    static const _MiscTags  xmptags[] = {
	{ "  XMP Rating", "Xmp.xmp.Rating", &data.rating },
	{ "  XMP Lens name", "Xmp.aux.Lens", &lens, NULL, NULL },
	{ NULL, NULL, NULL }
    };

    try
    {
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
	Exiv2::Image::UniquePtr image;
#else
	Exiv2::Image::AutoPtr image;
#endif
	try
	{
	    image = Exiv2::ImageFactory::open(filename_);
	
	}
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
	catch (const Exiv2::Error&)
#else
	catch (const Exiv2::AnyError&)
#endif
	{
	    std::ostringstream  err;
	    err << "invalid image file " << filename_;
	    throw std::invalid_argument(err.str());
	}

        /* determine if the img has an embedded prev image
         * http://www.exiv2.org/doc/classExiv2_1_1Image.html
         */
	bool  skiporientation = false;
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,26,0)
	switch (image->imageType())
	{
	    case Exiv2::ImageType::tiff:
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
	    case Exiv2::ImageType::nef:
#endif
	    case Exiv2::ImageType::cr2:
	    case Exiv2::ImageType::raf:
		data.type = ImgData::EMBD_PREVIEW;
		break;

	    case Exiv2::ImageType::jpeg:
	    case Exiv2::ImageType::jp2:
	    case Exiv2::ImageType::png:
	    case Exiv2::ImageType::gif:
		data.type = ImgData::IMAGE;
		break;
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,27,4)
	    case Exiv2::ImageType::bmff:
		data.type = ImgData::IMAGE;
		skiporientation = true;
		break;
#endif
	}
#else
	if (dynamic_cast<Exiv2::TiffImage*>(image.get()) ||    // nef
            dynamic_cast<Exiv2::Cr2Image *>(image.get()) ||    // canon
            dynamic_cast<Exiv2::RafImage *>(image.get())       // fuji
           )
        {
	    data.type = ImgData::EMBD_PREVIEW;
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
	{
	    std::ostringstream  os;
	    os << image->pixelWidth() << 'x' << image->pixelHeight();
	    data.xy = std::move(os.str());
	}
	const Exiv2::ExifData&  ed = image->exifData();
	if (ed.empty())
	{
	    // need to generate uniq key, will use ino
	    if (thumbpath_) {
		std::ostringstream  tmp;
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
        if (!data.metaimg.focallen.empty()) {
            // exiv2 gives this back like "105.0 mm" - round the focal length
            data.metaimg.focallen = std::to_string((unsigned)std::round(std::stof(data.metaimg.focallen))) + " mm";
        }

	const _MiscTags*  mp = misctags;
	while (mp->desc)
	{
	    if ( (exif = ed.findKey(Exiv2::ExifKey(mp->tag)) ) != ed.end()) {
		if (mp->target)    *mp->target   = exif->print(&ed);
		if (mp->tgtlong)   *mp->tgtlong  = 
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
		    exif->toInt64();
#else
		    exif->toLong();
#endif
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
	bool  flop = false;
	switch ((int)orientation) {
	    case 2:  orientation =   -0;  flop = true; break;   // top right, flip horz/mirrored
	    case 3:  orientation =  180;  break;                // bottom right/upside down
	    case 4:  orientation =  180;  flop = true; break;   // bottom left/upsided down mirrored, flip vert
	    case 5:  orientation =   90;  flop = true; break;   // left top, transpose/back to front/side
	    case 6:  orientation =   90;  break;                // right top
	    case 7:  orientation =  270;  flop = true; break;   // right bottom, traverse
	    case 8:  orientation =  270;  break;                // left bottom
	    case 1:
	    default: orientation =    0;  break;                // top left
	}
	if (!skiporientation) {
	    data.metaimg.rotate = orientation;
	    data.metaimg.flop = flop;
	}

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
	// special hack for lens names
	if (!lens.empty()) {
	    data.metaimg.lens = std::move(lens);
	}

	if (thumbpath_)
	{
	    data.thumb = thumbpath_;
	    if (data.thumb[data.thumb.length()-1] != '/') {
		data.thumb.append("/");
	    }

	    std::ostringstream  id;
	    if (data.metaimg.sn.empty() && data.metaimg.shuttercnt.empty()) {
		id << st_.st_dev << "-" << st_.st_ino;
	    }
	    else {
		// this should incl the model too
		id << data.metaimg.sn << '-' << data.metaimg.shuttercnt;
	    }
	    data.thumb.append(id.str());
	}

	return Img(ImgKey(mftr.c_str(), data.metaimg.camera.c_str(), data.metaimg.sn.c_str(), dtorg.c_str(), dtorgsub.c_str()), data);
    }
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
    catch (const Exiv2::Error& e)
#else
    catch (const Exiv2::AnyError& e)
#endif
    {
	std::ostringstream  err;
	err << "unable to extract exif from " << filename_ << " - " << e;
	throw std::underflow_error(err.str());
    }
}
