/*  $Id: imgprextr.cc,v 1.6 2012/04/07 13:39:17 ray Exp $
 *
 *  $Log: imgprextr.cc,v $
 *  Revision 1.6  2012/04/07 13:39:17  ray
 *  add -O for resize upon extract
 *
 *  Revision 1.5  2011/12/28 09:04:17  ray
 *  use SampleICC for ICC info
 *
 *  Revision 1.4  2011/12/27 12:04:42  ray
 *  allow arbituary ICC profiles along with internal sRGB
 *
 *  Revision 1.3  2011/12/01 13:57:30  ray
 *  maintain original exif byte order
 *
 *  Revision 1.2  2011/10/30 17:42:07  ray
 *  cvs tags
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <limits.h>
#include <libgen.h>

#include <string>
#include <iostream>
#include <cassert>

#include <fstream>
#include <sstream>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>

#include <exiv2/exiv2.hpp>
#include <Magick++.h>
#ifdef HAVE_SAMPLE_ICC
#include <SampleICC/IccProfile.h>
#include <SampleICC/IccTag.h>
#include <SampleICC/IccUtil.h>
#endif

/*
    g++ -O3 -s -DHAVE_SAMPLE_ICC -I/usr/local/include \
      imgprextr.cc ICCprofiles.c \
	    $(/usr/local/bin/Magick++-config --cppflags --cxxflags --libs) \
	    -lexiv2 -lexpat -lz -lSampleICC \
      -o imgprextr.exe
*/

#ifdef NEED_UCHAR_UINT_T
typedef unsigned char  uchar_t;
typedef unsigned int   uint_t;
#endif

#include "ICCprofiles.h"
#include "version.h"

#include "ImgBuf.h"


Exiv2::ExifData::iterator  _extractICC(ImgCat::_Buf& buf_, Exiv2::ExifData& exif_)
{
    buf_.clear();

    //const char*  iccproftag = "Exif.Image.0x8773";
    const char*  iccproftag = "Exif.Image.InterColorProfile";

    Exiv2::ExifData::iterator  d;
    if ( (d = exif_.findKey(Exiv2::ExifKey(iccproftag)) ) != exif_.end()) {
	const Exiv2::Value&  val = d->value();
	buf_.alloc(d->size());
	d->copy(buf_.buf, Exiv2::invalidByteOrder);
    }
    else
    {
	bool  justdump = true;

	/* no embedded ICC so can't do any conversion, just dump the preview
	 * image if its not a Nikon RAW with the colr space set
	 */
	if ( (d = exif_.findKey(Exiv2::ExifKey("Exif.Nikon3.ColorSpace")) ) != exif_.end()) 
	{
	    /* found the nikon tag that tells us 0=srgb, 1=argb but need to check if
	     * this is as-shot with no further mods (ie colorspace conv)
	     */
	    if (d->toLong() == 2)
	    {
		// it was shot as aRGB - check the orig vs mod times
		std::string  orig;
		std::string  mod;

		Exiv2::ExifData::iterator  e;
		if ( (e = exif_.findKey(Exiv2::ExifKey("Exif.Image.DateTime")) ) != exif_.end()) {
		    mod = e->toString();
		}
		if ( (e = exif_.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal")) ) != exif_.end()) {
		    orig = e->toString();
		}

		if (orig == mod) {
		    // ok, we'll assume this is really still in aRGB
		    justdump = false;
		}
	    }
	}

	if (justdump) {
	    d = exif_.end();
	}
	else {
	    buf_.alloc(theNkaRGBicc->length);
	    memcpy(buf_.buf, theNkaRGBicc->profile, theNkaRGBicc->length);
	}
    }

    return d;
}


void  _extractProfile(const void* data_, const size_t datasz_)
{
    try
    {
	Magick::Blob   blob(data_, datasz_);
	Magick::Image  magick(blob);

	Magick::Blob   profile = magick.iccColorProfile();
	std::cout << "data size = " << datasz_ << std::endl;
	std::cout << "profile size = " << profile.length() << std::endl;
	if (profile.length() == 0) {
#if 0
	    // ??? lets be trying ICC/ICM
	    const Magick::Blob   icc  = magick.profile("ICC");
	    const Magick::Blob   icm  = magick.profile("ICM");

	    std::cout << "  explicit icc len=" << icc.length() << std::endl
	         << "  explicit icm len=" << icm.length() << std::endl;

	    mode_t  msk = umask(0);
	    umask(msk);
	    int  fd;
	    if ( (fd = open("icc.icc", O_CREAT | O_TRUNC, 0666 & ~msk)) < 0) {
		std::cerr << "  failed to create icc profile - " << strerror(errno) << std::endl;
	    }
	    else 
	    {
		if ( write(fd, icc.data(), icc.length()) != profile.length()) {
		    std::cerr << "  failed to write profile data - " << strerror(errno) << std::endl;
		}
	    }
	    close(fd);

	    if ( (fd = open("icm.icc", O_CREAT | O_TRUNC, 0666 & ~msk)) < 0) {
		std::cerr << "  failed to create icm profile - " << strerror(errno) << std::endl;
	    }
	    else 
	    {
		if ( write(fd, icm.data(), icm.length()) != profile.length()) {
		    std::cerr << "  failed to write profile data - " << strerror(errno) << std::endl;
		}
	    }
	    close(fd);
#endif
	    return;
	}

	mode_t  msk = umask(0);
	umask(msk);
	int  fd;
	if ( (fd = open("profile.icc", O_CREAT | O_TRUNC, 0666 & ~msk)) < 0) {
	    std::cerr << "  failed to create profile - " << strerror(errno) << std::endl;
	    return;
	}

	if ( write(fd, profile.data(), profile.length()) != profile.length()) {
	    std::cerr << "  failed to write profile data - " << strerror(errno) << std::endl;
	}
	close(fd);
    }
    catch (const std::exception& ex)
    {
	std::cerr << "failed to magick - " << ex.what() << std::endl;
    }
}

struct PrvInfo {
    std::string  dateorig;
    std::string  mftr;
    std::string  camera;
    std::string  sn;
    std::string  shuttercnt;

    const char* seperator;

    PrvInfo(const Exiv2::Image&);

    PrvInfo(PrvInfo&& rhs_) {
        seperator = rhs_.seperator;
        dateorig = std::move(rhs_.dateorig);
        mftr = std::move(rhs_.mftr);
        camera = std::move(rhs_.camera);
        sn = std::move(rhs_.sn);
        shuttercnt = std::move(rhs_.shuttercnt);
    }

    PrvInfo(const PrvInfo&) = delete;
    PrvInfo& operator=(const PrvInfo&) = delete;
    PrvInfo& operator=(PrvInfo&&) = delete;
};

std::ostream& operator<<(std::ostream& os_, const PrvInfo& obj_)
{
    const char*  s = obj_.seperator ? obj_.seperator : " ";
    std::string  tmp;
    auto  fmt = [&s](const std::string& s_, bool first_=false) {
        std::ostringstream  f;
        if (s_.empty()) {
            return s_;
        }
        if (first_) {
            f << s_;
        }
        else {
            f << s << s_;
        }
        return f.str();
    };
    return os_ << fmt(obj_.dateorig, true) << fmt(obj_.mftr) << fmt(obj_.camera) << fmt(obj_.sn) << fmt(obj_.shuttercnt);
}

PrvInfo::PrvInfo(const Exiv2::Image& img_) : seperator(nullptr)
{
    typedef Exiv2::ExifData::const_iterator (*EasyAccessFct)(const Exiv2::ExifData& ed);
    const struct _EasyAccess {
	EasyAccessFct find;
	std::string*  target;
    } eatags[] = {
	{ Exiv2::make,         &mftr   },
	{ Exiv2::model,        &camera },
	{ Exiv2::serialNumber, &sn     },
	{ NULL, NULL }
    };

    const struct _MiscTags {
        const char*  tag;
        std::string*  s;
    } misctags[] = {
        { "Exif.Photo.DateTimeOriginal", &dateorig },
        { "Exif.Nikon3.ShutterCount",    &shuttercnt },
        { NULL, NULL }
    };

    const Exiv2::ExifData&  ed = img_.exifData();
    if (ed.empty()) {
        return;
    }

    Exiv2::ExifData::const_iterator  exif;
    const _EasyAccess*  ep = eatags;
    while (ep->target)
    {
	if ( (exif = ep->find(ed)) != ed.end())  {
	    *ep->target = exif->print(&ed);
	}
	++ep;
    }

    if (mftr == "NIKON CORPORATION") {
        mftr.clear();  // the model has "NIKON ..."
    }
    else {
        if (mftr.length() > 10) {
            mftr.replace(10, mftr.length(), 3, '.');
        }
    }

    const _MiscTags*  mp = misctags;
    while (mp->tag)
    {
        if ( (exif = ed.findKey(Exiv2::ExifKey(mp->tag)) ) != ed.end()) {
            *mp->s = exif->print(&ed);
        }
        ++mp;
    }
}


bool  _extractICCinfo(const void* data_, const size_t datasz_, std::string& desc_, std::string& cprt_)
{
    CIccProfile*  icc = OpenIccProfile((icUInt8Number*)data_, datasz_);
    if (icc == NULL) {
	return false;
    }

    CIccTag*  tag = NULL;

    if ( (tag = icc->FindTag(icSigProfileDescriptionTag))) {
	if ( tag->GetType() == icSigTextDescriptionType) {
	    CIccTagTextDescription*  txt = (CIccTagTextDescription*)tag;
	    desc_ = txt->GetText();
	}
    }

    if ( (tag = icc->FindTag(icSigCopyrightTag))) {
	if (tag->GetType() == icSigTextType) {
	    CIccTagText*  txt = (CIccTagText*)tag;
	    cprt_ = txt->GetText();
	}
    }

    delete icc;
    return true;
}


std::string  generate_hex(const unsigned int len_)
{
    std::stringstream  ss;
    std::random_device  rd;

    for (auto i = 0; i < len_; i++) {
        std::stringstream  hexstream;

	std::mt19937  gen(rd());
	std::uniform_int_distribution<>  dis(0, 255);

        hexstream << std::hex << dis(gen);
        auto hex = hexstream.str();
        ss << (hex.length() < 2 ? '0' + hex : hex);
    }
    return ss.str();
}


int main(int argc, char* const argv[])
{
    const char*  argv0 = basename(argv[0]);

    const char*  thumbpath = "./";
    bool  dumpICC = false;
    bool  excludeMeta = false;
    short convert = 0;
#define CONVERT_ICC 1
#define CONVERT_OUTPUT_FMT 2
    std::string  outputfmt;

    const ICCprofiles*  tgtICC     = NULL;  // used to determine if ICC conversions req'd
    uchar_t*            nonSRGBicc = NULL;  // buf for non internal sRGB ICC

    Magick::InitializeMagick(NULL);

    Magick::Geometry  target;
    enum FilenameType { FNT_FILE, FNT_RAND, FNT_META };
    FilenameType  fnt = FNT_FILE;
    int  imgqual = 100;
    const auto  maxtpsz = std::thread::hardware_concurrency()*3;
    const auto  dflttpsz = ceil(std::thread::hardware_concurrency()/2);
    unsigned  tpsz = dflttpsz;
    std::list<std::future<void>>  futures;

    int  c;
    while ( (c=getopt(argc, argv, "p:Ic:xhO:o:q:RMT:")) != EOF) {
	switch (c)
	{
	    case 'p':
		thumbpath = optarg;
		break;

	    case 'q':
		imgqual = atol(optarg);
		break;

	    case 'R':
		fnt = FNT_RAND;
		break;

            case 'M':
                fnt = FNT_META;
                break;

            case 'T':
            {
                tpsz = (unsigned)atol(optarg);
                if (tpsz > maxtpsz) {
                    tpsz = maxtpsz;
                }
            } break;

	    case 'c':
	    {
		if (strcasecmp(optarg, "srgb") == 0) {
		    tgtICC = theSRGBICCprofiles;
		}
		else
		{
		    tgtICC = theSRGBICCprofiles;
		    while (tgtICC->profile) {
			if ( strcmp(tgtICC->name, optarg) == 0) {
			    break;
			}
			++tgtICC;
		    }

		    if (tgtICC->profile == NULL)
		    {
			tgtICC = NULL;

			// not a trgt ICC that we know of internally
			struct stat  st;
			if (stat(optarg, &st) < 0) {
			    std::cerr << argv0 << ": invalid ICC " << "'" << optarg << "' - " << strerror(errno) << std::endl;
			    return 1;
			}

			if (st.st_size == 0 || S_ISREG(st.st_mode) == 0) {
			    std::cerr << argv0 << ": invalid ICC " << "'" << optarg << "'" << std::endl;
			    return 1;
			}

			int  fd;
			if ( (fd = open(optarg, O_RDONLY)) > 0)
			{
			    nonSRGBicc = new uchar_t[sizeof(struct ICCprofiles) + st.st_size + strlen(optarg)+1];
			    memset(nonSRGBicc, 0, sizeof(struct ICCprofiles) + st.st_size + strlen(optarg)+1);

			    uchar_t*&  profile = (uchar_t*&)((ICCprofiles*)nonSRGBicc)->profile;
			    profile = (uchar_t*)((char*)nonSRGBicc + sizeof(struct ICCprofiles));
			    if (read(fd, profile, st.st_size) == st.st_size) {
				size_t&  tmp = (size_t&)(((ICCprofiles*)nonSRGBicc)->length);
				tmp = st.st_size;

				char*&  name = (char*&)(((ICCprofiles*)nonSRGBicc)->name);
				name = (char*)nonSRGBicc + sizeof(struct ICCprofiles) + st.st_size;
				strcpy(name, optarg);

				tgtICC = (ICCprofiles*)nonSRGBicc;
			    }
			    else {
				std::cerr << argv0 << ": unable to read ICC '" << optarg << "' - " << strerror(errno) << std::endl;
				delete [] nonSRGBicc;
				nonSRGBicc = NULL;
			    }
			}
			close(fd);
		    }

		    if (tgtICC == NULL) {
			goto usage;
		    }
		    convert |= CONVERT_ICC;
		}
	    } break;

	    case 'I':
		dumpICC = true;
		break;

	    case 'x':
		excludeMeta = true;
		break;

	    case 'O':
		try
		{
		    const Magick::Geometry  g(optarg);
		    const std::string  s = g;
		    target = s.c_str();
		}
		catch (...)
		{ }
		break;

	    case 'o':
		if (strcasecmp(optarg, "JPEG") == 0 || strcasecmp(optarg, "PNG") == 0) {
		    convert |= CONVERT_OUTPUT_FMT;
		    outputfmt = optarg;
		    std::transform(outputfmt.begin(), outputfmt.end(), outputfmt.begin(), ::toupper);
		}
		else {
		    if (strcasecmp(optarg, "DEFAULT") != 0) {
			std::cerr << argv0 << ": unsupported output format '" << optarg << "', will not convert output" << std::endl;
		    }
		}
		break;

	    case 'h':
	    default:
usage:
		std::cout << argv0 << " " << Imgcat::version() << "\n"
		     << "usage: " << argv0 << " [ -p path ] [-c <target ICC profile location> | srgb] [-x] [-I] [-O <output size>] [-o JPEG | PNG | ORIG] [-q quality] [-R|-M]  file0 file1 .. fileN" << std::endl
		     << "         -p    extract preview images to location=./" << std::endl
		     << "         -c    perform ICC conversion if possible: srgb for internal sRGB or file location of target ICC" << std::endl
		     << "         -x    exclude metadata" << std::endl
		     << "         -I    dump ICC to disk for each image" << std::endl
		     << "         -O    target (re)size" << std::endl
		     << "         -o    target output format" << std::endl
		     << "         -q    quality" << std::endl
		     << "         -R    random 16 byte hex for filename output (not incl extn)" << std::endl
		     << "         -M    meta info used for filename output (not incl extn)" << std::endl
		     << "         -T    threads to use (default=" << tpsz << " h/w=" << std::thread::hardware_concurrency() << " max=" << maxtpsz << ')' << std::endl
		     << "  internal ICC profiles: ";

		const ICCprofiles*  p = theSRGBICCprofiles;
		while (p->profile) {
		    std::cout << " '" << p->name << "' (" << p->length << "bytes)";
		    ++p;
		}
		std::cout << std::endl;
		return 1;
	}
    }
    mode_t  msk = umask(0);
    umask(msk);

    if (tpsz < 1) {
        tpsz = dflttpsz;
    }


    // make sure thumbpath is writable
    errno = 0;
    if (access(thumbpath, X_OK | W_OK) != 0)
    {
	if (errno == ENOENT) {
#ifdef __MINGW32__
	    if (mkdir(thumbpath) < 0) {
#else
	    mode_t  umsk = umask(0);
	    umask(umsk);
	    if (mkdir(thumbpath, 0777 & ~umsk) < 0) {
#endif
		goto thumbpatherr;
	    }
	}
	else {
thumbpatherr:
	    std::cerr << argv0 << ": invalid thumbpath '" << thumbpath << "' - " << strerror(errno) << std::endl;
	    return 1;
	}
    }


    if (optind == argc) {
	std::cerr << argv0 << ": no input files" << std::endl;
	goto usage;
    }

    if (tgtICC == NULL) {
	std::cout << argv0 << ": preview extract only (no ICC conversions)" << std::endl;
    }
    else
    {
	std::cout << argv0 << ": ICC target=" << (nonSRGBicc ? "external" : "internal") << " '" << tgtICC->name << "'";

#ifdef HAVE_SAMPLE_ICC
	std::string  tgtICCdesc = "<no internal desc>";
	std::string  tgtICCcprt = "<no internal copyright>";

	if ( !_extractICCinfo(tgtICC->profile, tgtICC->length, tgtICCdesc, tgtICCcprt)) {
	    std::cerr << "  invalid ICC fmt" << std::endl;
	    return -1;
	}
	std::cout << "  " << tgtICCdesc << " (c) " << tgtICCcprt << std::endl;
#else
	std::cout << std::endl;
#endif
    }

    const Magick::Blob*  outicc = tgtICC ? new Magick::Blob(tgtICC->profile, tgtICC->length) : NULL;

    std::mutex  mtx;
    std::condition_variable  cond;
    unsigned  sem = tpsz;

    std::ofstream  devnull("/dev/null");
    std::streambuf*  erros = std::cerr.rdbuf(devnull.rdbuf());

    int  a = optind;
    while (a < argc)
    {
	const char* const  filename = argv[a++];

        auto  task = [&](const char* filename_) {
            struct Raii
            {
                std::mutex&  mtx;
                std::condition_variable&  cond;
                unsigned&  sem;

                Raii(std::mutex& mtx_, std::condition_variable& cond_, unsigned& sem_)
                    : mtx(mtx_), cond(cond_), sem(sem_)
                {
                    std::unique_lock<std::mutex>  lck(mtx);
                    cond.wait(lck, [&sem  = sem]() { return sem > 0; });
                    --sem;
                    lck.unlock();
                }

                ~Raii()
                {
                    mtx.lock();
                    ++sem;
                    cond.notify_all();
                    mtx.unlock();
                }

                Raii(const Raii&) = delete;
                Raii(Raii&&) = delete;

                Raii& operator=(const Raii&) = delete;
                Raii& operator=(Raii&&) = delete;
            };
            const Raii  raii(mtx, cond, sem);
            std::ostringstream  err;

            ImgCat::_Buf  buf(1024);
            int  fd;

            try
            {
                Exiv2::Image::AutoPtr  orig = Exiv2::ImageFactory::open(filename_);
                assert(orig.get() != 0);


                orig->readMetadata();

                Exiv2::PreviewManager loader(*orig);
                Exiv2::PreviewPropertiesList  list = loader.getPreviewProperties();

                // grabbing the largest preview
                Exiv2::PreviewPropertiesList::iterator prevp = list.begin();
                if (prevp == list.end()) {
                    err << filename_ << ":  no preview";
                    throw std::underflow_error(err.str());
                }

                advance(prevp, list.size()-1);

                PrvInfo  pvi(*(orig.get()));

                char  path[PATH_MAX];
                char  path1[PATH_MAX];
                switch (fnt) {
                    case FNT_RAND:
                        strcpy(path1, generate_hex(16).c_str());
                        break;

                    case FNT_META:
                    {
                        std::stringstream  d;
                        const char*  osep = pvi.seperator;
                        pvi.seperator = "-";
                        d << pvi;
                        strcpy(path1, d.str().c_str());
                        pvi.seperator = osep;
                    } break;

                    default:
                        strcpy(path1, filename_);
                }
                sprintf(path, "%s/%s", thumbpath, basename(path1));

#define LOG_FILE_INFO  filename_ << ": " << std::setw(8) << prevp->size_ << " bytes, " << prevp->width_ << "x" << prevp->height_ << "  " << pvi

                std::cout << LOG_FILE_INFO << std::endl;

                Exiv2::PreviewImage  preview = loader.getPreviewImage(*prevp);

                Exiv2::Image::AutoPtr  upd = Exiv2::ImageFactory::open( preview.pData(), preview.size() );

                if (!excludeMeta) {
                    upd->setByteOrder(orig->byteOrder());

                    upd->setExifData(orig->exifData());
                    upd->setIptcData(orig->iptcData());
                    upd->setXmpData(orig->xmpData());

                    upd->writeMetadata();
                }

                Exiv2::ExifData&  exif = orig->exifData();
                Exiv2::ExifData::iterator  iccavail = _extractICC(buf, exif);

                if (iccavail != exif.end() && tgtICC)
                {
                    if (dumpICC)
                    {
                        char  path[PATH_MAX];
                        sprintf(path, "%s/%s.icc", thumbpath, basename(path1));

#ifdef __MINGW32__
                        if ( (fd = open(path, O_CREAT | O_WRONLY | O_BINARY, 0666 & ~msk)) < 0) {
#else
                        if ( (fd = open(path, O_CREAT | O_WRONLY, 0666 & ~msk)) < 0) {
#endif
                            std::cout << LOG_FILE_INFO << ": failed to create ICC - " << strerror(errno) << std::endl;
                        }
                        else
                        {
                            if (write(fd, buf.buf, buf.bufsz) != buf.bufsz) {
                                std::cout << LOG_FILE_INFO << ": failed to write ICC - " << strerror(errno) << std::endl;
                            }
                            close(fd);
                        }
                    }

                    if (nonSRGBicc == NULL)
                    {
                        /* is this ICC profile sRGB - covnert only if not; the
                         * outicc is already pointing at a sRGB icc
                         */
                        const ICCprofiles*  p = theSRGBICCprofiles;
                        while (p->profile)
                        {
                            if (p->length == buf.bufsz && memcmp(p->profile, buf.buf, p->length) == 0) {
                                break;
                            }
                            ++p;
                        }

                        if (p->profile) {
                            std::cout << argv0 << ": " << LOG_FILE_INFO << ": already sRGB (" << p->name << ") - not converting" << std::endl;
                            convert &= ~CONVERT_ICC;
                        }
                        else
                        {
                            /* the ICC profile is different to the any of the known
                             * sRGBs so we'll have to convert
                             */
                            convert |= CONVERT_ICC;
                        }
                    }
                }


                if (convert)
                {
                    try
                    {
                        if (!excludeMeta && iccavail != exif.end() ) {
                            exif.erase(iccavail);
                        }

                        /* grab hold of underlying and updated data */
                        Exiv2::BasicIo&  rawio = upd->io();
                        rawio.seek(0, Exiv2::BasicIo::beg);

                        Magick::Image  img(Magick::Blob(rawio.mmap(), rawio.size()));

                        img.quality(imgqual);
                        if (convert & CONVERT_OUTPUT_FMT) {
                            char  extn[5];
                            if      (strcasecmp(outputfmt.c_str(), "JPEG") == 0) sprintf(extn, ".jpg");
                            else if (strcasecmp(outputfmt.c_str(), "PNG")  == 0) sprintf(extn, ".png");
                            else abort();
                            strcat(path, extn);
                            img.magick(outputfmt);
                        }
                        else {
                            strcat(path, preview.extension().c_str());
                        }

                        if (convert & CONVERT_ICC)
                        {
                            try
                            {
                                short  cretry = 5;
                                while (cretry--) {
                                    try
                                    {
                                        img.iccColorProfile(*outicc);
                                        break;
                                    }
                                    // smoetimes we get this:
                                    // ColorspaceColorProfileMismatch `ICC' @ error/profile.c/ProfileImage/829
                                    catch (const Magick::ErrorResourceLimit&)
                                    {
                                        std::this_thread::sleep_for(std::chrono::milliseconds(333));
                                    }
                                }
                            }
                            catch (const Magick::Exception& ex)
                            {
                                std::cout << "failed to convert ICC, ignoring - " << ex.what() << "\n";
                            }
                        }
                        // exif not maintained!!!

#ifdef HAVE_SAMPLE_ICC
                        std::string  desc = "<unknown>", cprt;
                        _extractICCinfo(buf.buf, buf.bufsz, desc, cprt);
#endif

                        std::cout << LOG_FILE_INFO << ": ICC converted";
#ifdef HAVE_SAMPLE_ICC
                        std::cout << " from " << desc;
#endif
                        std::cout << std::endl;
                        if (target.isValid()) {
                            img.resize(target);
                        }

                        if (excludeMeta) {
                            img.write(path);
                        }
                        else {
                            // because IM doesnt take across the Exif data, take the converted img back to Exiv2
                            Magick::Blob  blob;
                            img.write(&blob);

                            Exiv2::Image::AutoPtr  cnvrted = Exiv2::ImageFactory::open((const unsigned char*)blob.data(), blob.length());
                            cnvrted->readMetadata();
                            cnvrted->setByteOrder(orig->byteOrder());
                            cnvrted->setExifData(exif);
                            cnvrted->writeMetadata();

#ifdef __MINGW32__
                            if ( (fd = open(path, O_CREAT | O_WRONLY | O_BINARY, 0666 & ~msk)) < 0) {
#else
                            if ( (fd = open(path, O_CREAT | O_WRONLY, 0666 & ~msk)) < 0) {
#endif
                                err << LOG_FILE_INFO << ": failed to create converted preview - " << strerror(errno) << std::endl;
                                throw std::system_error(errno, std::system_category(), err.str());
                            }

                            Exiv2::BasicIo&  rawio = cnvrted->io();
                            rawio.seek(0, Exiv2::BasicIo::beg);

                            if (write(fd, rawio.mmap(), rawio.size()) != rawio.size()) {
                                close(fd);
                                err << LOG_FILE_INFO << ": failed to write converted preview - " << strerror(errno) << std::endl;
                                throw std::system_error(errno, std::system_category(), err.str());
                            }
                            close(fd);
                        }
                    }
                    catch (const std::exception& ex)
                    {
                        throw;
                    }
                }
                else
                {
                    strcat(path, preview.extension().c_str());
#ifdef __MINGW32__
                    if ( (fd = open(path, O_CREAT | O_WRONLY | O_BINARY, 0666 & ~msk)) < 0) {
#else
                    if ( (fd = open(path, O_CREAT | O_WRONLY, 0666 & ~msk)) < 0) {
#endif
                        err << LOG_FILE_INFO << ": failed to create preview - " << strerror(errno);
                        throw std::system_error(errno, std::system_category(), err.str());
                    }

                    Exiv2::BasicIo&  rawio = upd->io();
                    rawio.seek(0, Exiv2::BasicIo::beg);

                    if (write(fd, rawio.mmap(), rawio.size()) != rawio.size()) {
                        close(fd);
                        err << LOG_FILE_INFO << ": failed to write converted preview - " << strerror(errno) << std::endl;
                        throw std::system_error(errno, std::system_category(), err.str());
                    }
                    close(fd);
                }
            }
            catch (const Exiv2::AnyError& e)
            {
                err << filename_ << ":  unable to extract preview/reset exif - " << e;
                throw std::overflow_error(err.str());
            }
        };

        try
        {
            futures.emplace_back(std::async(std::launch::async, task, filename));
        }
        catch (const std::system_error& ex)
        {
            std::cout << "failed to start thread - " << ex.what() << std::endl;
            break;
        }
    }

    // wait for all threads to finish
    try
    {
        std::unique_lock<std::mutex>  lck(mtx);
        cond.wait(lck, [&sem, &tpsz]() { return sem == tpsz; });
        lck.unlock();
        std::cerr.rdbuf(erros);
        for (auto& f : futures) {
            try {
                f.get();
            }
            catch (const std::exception& ex) {
                std::cerr << argv0 << ": " << ex.what() << std::endl;
            }
        }
    }
    catch (const std::system_error& ex) {
        std::cerr << argv0 << ": failed to clean up threads - " << ex.what() << std::endl;
    }

    delete [] nonSRGBicc;
    delete outicc;
    Magick::TerminateMagick();

    return 0;
}
