#include "ImgThumbGen.h"

#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>

#include <Magick++/Exception.h>
#include <libffmpegthumbnailer/videothumbnailer.h>
#include <libffmpegthumbnailer/filmstripfilter.h>

#include "ICCprofiles.h"
#include "ImgBuf.h"


#ifdef DEBUG_LOG
#include <iostream>
#define DLOG(x)  std::cout << __FILE__ << ",line " << __LINE__ << " DEBUG:  " << x << std::endl;
#else
#define DLOG(x)
#endif


std::once_flag  ImgThumbGen::_once;


void  ImgThumbGen::generate()
{
    gettimeofday(&_x, NULL);
    const ImgData&  img = _img;

    /* grab the exif and thumb from the very first item which is
     * supposed to be the primary image
     */

    // explicity want a jpg
    const std::string  prevpath = _prevpath;
    try
    {
        if (img.type == ImgData::EMBD_PREVIEW)
        {
            /* this is most likely a raw file which has embedded thumbs
             */
            try
            {
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
                Exiv2::Image::UniquePtr
#else
                Exiv2::Image::AutoPtr
#endif
		orig = Exiv2::ImageFactory::open(img.filename);
                orig->readMetadata();
                Exiv2::PreviewManager  prevldr(*orig);
                Exiv2::PreviewPropertiesList  prevs = prevldr.getPreviewProperties();
                if (prevs.empty()) {
                    _readgenthumbnail(img, prevpath, thumbsize);
                }
                else
                {
                    // get the largest, convert to sRGB if possible and scale
                    const Exiv2::PreviewImage  preview = prevldr.getPreviewImage(prevs.back());

                    _genthumbnail(prevpath, img.filename, preview, orig->exifData(), thumbsize, img.metaimg.rotate, img.metaimg.flop);
                }
            }
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
	    catch (const Exiv2::Error& e)
#else
	    catch (const Exiv2::AnyError& e)
#endif
            {
                _error << "unable to extract thumbnail from " << img.filename << " - " << e;
            }
        }
        else
        {
            if (img.type == ImgData::IMAGE)
            {
                _readgenthumbnail(img, prevpath, thumbsize);
            }
            else if (img.type == ImgData::VIDEO)
            {
                ffmpegthumbnailer::VideoThumbnailer videoThumbnailer(thumbsize, true, true, 10, false);
                ffmpegthumbnailer::FilmStripFilter  filmStripFilter;
                videoThumbnailer.addFilter(&filmStripFilter);

                videoThumbnailer.setSeekPercentage(10);
                char  hack[PATH_MAX+1];
                sprintf(hack, "%s.jpg", img.thumb.c_str());
                videoThumbnailer.generateThumbnail(img.filename.c_str(), Jpeg, hack);
            }
            else {
                _error << "unknown source file type, not attempting to generate thumbnail - " << img.filename;
            }
        }
    }
    catch (const std::exception& e)
    {
        _error << e.what();
    }

    gettimeofday(&_y, NULL);
}




void  ImgThumbGen::_genthumbnail(const std::string& path_, const std::string& origpath_, Magick::Image& img_, const unsigned sz_, const float rotate_, const bool flop_)
{
    try
    {
	if (rotate_) {
	    img_.rotate(rotate_);
	}
	if (flop_) {
	    img_.flop();
	}

	/* make sure that the thumb is at least 'sz_'
	 * high 
	 */
        std::ostringstream  tmp;
	if (img_.columns() < img_.rows()) {
	    tmp << "x" << sz_;
	}
	else {
	    tmp << sz_ << "x";
	}
	img_.scale(tmp.str().c_str());

#if 0
	const size_t   r = img_.rows();
	const size_t   c = img_.columns();

	unsigned  roff = 0;
	unsigned  coff = 0;

	if (r == sz_ && c > sz_) {
	    coff = (c - sz_)/2;
	}
	else {
	    if (c == sz_ && r > sz_) {
		roff = (r - sz_)/2;
	    }
	}

	img_.crop( Magick::Geometry(sz_, sz_, coff, roff) );
#endif
	img_.write(path_.c_str());
    }
    catch (const std::exception& ex)
    {
	std::ostringstream  err;
	err << "failed to resize thumbnail " << origpath_ << " - " << ex.what();
	throw std::range_error(err.str());
    }
}

void  ImgThumbGen::_genthumbnail(const std::string& path_, const std::string& origpath_,
                                 const void* data_, const size_t datasz_, const unsigned sz_, const float rotate_, const bool flop_)
{
    try
    {
	DLOG("thumbnail path=" << path_ << " orig=" << origpath_);
        short  mgkretries = 3;
        while (mgkretries-- > 0)
        {
            try
            {
                Magick::Blob   blob(data_, datasz_);
                Magick::Image  magick(blob);
		magick.strip();

                _genthumbnail(path_, origpath_, magick, sz_, rotate_, flop_);
                break;
            }
            catch (const Magick::ErrorCache& ex) 
            {
                DLOG("IM cache error, orig=" << origpath_ << " - " << ex.what());
                if (mgkretries == 0) {
                    throw;
                }
                sleep(1);
            }
            catch (const Magick::Exception& ex)
            {
                DLOG("unexpected IM exception, orig=" << origpath_ << " typeid=" << typeid(ex).name() << " - " << ex.what());
                throw;
            }
        }
    }
    catch (const std::exception& ex)
    {
	std::ostringstream  err;
	err << "failed to resize thumbnail, unable to create internal obj " << origpath_ << " - " << ex.what();
	throw std::range_error(err.str());
    }
}


void  ImgThumbGen::_readgenthumbnail(const ImgData& img_, const std::string& prevpath_, const unsigned sz_)
{
    int  fd;
    if ( (fd = open(img_.filename.c_str(), O_RDONLY)) < 0) {
	std::cerr << "failed to open source file to generate thumbnail: " << img_.filename << " - " << strerror(errno) << std::endl;
    }
    else
    {
	errno = 0;
	char*  buf = new char[img_.size];
	size_t  n;
	if ( (n = read(fd, buf, img_.size)) != img_.size) {
	    std::cerr << "failed to read source file to generate thumbnail: " << img_.filename << " (read " << n << "/" << img_.size << ") - " << strerror(errno) << std::endl;
	}
	else {
	    _genthumbnail(prevpath_, img_.filename, buf, img_.size, sz_, img_.metaimg.rotate, img_.metaimg.flop);
	}
	delete []  buf;
	close(fd);
    }
}


void  ImgThumbGen::_genthumbnail(const std::string& path_, const std::string& origpath_,
                                 const Exiv2::PreviewImage& preview_, const Exiv2::ExifData& exif_, const unsigned sz_,
                                 const float rotate_, const bool flop_)
{
    const char*  iccproftag = "Exif.Image.InterColorProfile";

    static ImgCat::_Buf  buf(1024);

    bool  convertICC = false;

    Exiv2::ExifData::const_iterator  d;
    if ( (d = exif_.findKey(Exiv2::ExifKey(iccproftag)) ) == exif_.end())
    {
	bool  justdump = true;

	DLOG(origpath_ << "  no embedded icc");

	/* no embedded ICC so can't do any conversion, just dump the preview
	 * image if its not a Nikon RAW with the colr space set
	 */
	if ( (d = exif_.findKey(Exiv2::ExifKey("Exif.Nikon3.ColorSpace")) ) != exif_.end()) 
	{
	    /* found the nikon tag that tells us 0=srgb, 1=argb but need to check if
	     * this is as-shot with no further mods (ie colorspace conv)
	     */
	    const long  l =
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
	    d->toInt64();
#else
	    d->toLong();
#endif

	    DLOG(origpath_ << "  colourspace=" << l);
	    if (l == 2)
	    {
		// it was shot as aRGB - check the orig vs mod times
                std::string  orig;
                std::string  mod;

		DLOG(origpath_ << "  shot as aRGB");
		if ( (d = exif_.findKey(Exiv2::ExifKey("Exif.Image.DateTime")) ) != exif_.end()) {
		    mod = d->toString();
		}
		if ( (d = exif_.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal")) ) != exif_.end()) {
		    orig = d->toString();
		}

		if (orig == mod) {
		    // ok, we'll assume this is really still in aRGB
		    justdump = false;
		    DLOG(origpath_ << "  no mods, will convert from aRGB");
		}
	    }
	}

	if (justdump) {
	    buf.alloc(theNksRGBicc->length);
	    memcpy(buf.buf, theNksRGBicc->profile, theNksRGBicc->length);
	}
	else {
	    buf.alloc(theNkaRGBicc->length);
	    memcpy(buf.buf, theNkaRGBicc->profile, theNkaRGBicc->length);
	}
    }
    else
    {
	/* take the embedded ICC */
	buf.alloc(d->size());
	d->copy(buf.buf, Exiv2::invalidByteOrder);

	DLOG(origpath_ << "     embedded icc");
    }

    const ICCprofiles*  p = theSRGBICCprofiles;
    while (p->profile)
    {
	if (p->length == buf.bufsz && memcmp(p->profile, buf.buf, p->length) == 0) {
	    break;
	}
	++p;
    }

    if (p->profile)
    {
	DLOG(origpath_ << "  already sRGB");

	/* already a known sRGB, no conversion needed just dump the preview */
	_genthumbnail(path_, origpath_, preview_.pData(), preview_.size(), sz_, rotate_, flop_);
    }
    else
    {
	DLOG(origpath_ << "      not sRGB");

	static  const Magick::Blob  outicc(theSRGBICCprofiles[0].profile, theSRGBICCprofiles[0].length);

        short  mgkretries = 5;
        while (mgkretries-- > 0)
        {
            try
            {
                Magick::Image  img(Magick::Blob( preview_.pData(), preview_.size() ));
                img.profile("ICC", Magick::Blob(buf.buf, buf.bufsz));
                img.profile("ICC", outicc);

                _genthumbnail(path_, origpath_, img, sz_, rotate_, flop_);
                break;
            }
            catch (const Magick::ErrorCache& ex)
            {
                DLOG("IM cache error, orig=" << origpath_ << " - " << ex.what());
                if (mgkretries == 0) {
                    throw;
                }
                sleep(1);
            }
            catch (const Magick::Exception& ex)
            {
                // most likely Magick: ColorspaceColorProfileMismatch 
                DLOG("unexpected IM exception, orig=" << origpath_ << " typeid=" << typeid(ex).name() << " - " << ex.what());
                throw;
            }
        }
    }
}
