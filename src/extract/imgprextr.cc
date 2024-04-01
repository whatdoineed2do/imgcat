#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <limits.h>
#include <libgen.h>
#include <getopt.h>

#include <string>
#include <iostream>
#include <cassert>
#include <array>
#include <algorithm>

#include <fstream>
#include <sstream>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <exception>
#include <typeinfo>

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
#endif

#include "ICCprofiles.h"
#include "version.h"

#include "ImgBuf.h"
#include "PreviewInfo.h"


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
            const long  l =
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
		    d->toInt64();
#else
		    d->toLong();
#endif
	    if (l == 2)
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


bool  _extractICCinfo(const void* data_, const size_t datasz_, std::string& desc_, std::string& cprt_)
{
    std::unique_ptr<CIccProfile>  icc(OpenIccProfile((icUInt8Number*)data_, datasz_));
    if (icc.get() == NULL) {
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

struct ImgSize {
    ImgSize(const char* name_, unsigned x_, unsigned y_)
        : name(name_), x(x_), y(y_)
    { }
    ImgSize(const ImgSize&) = default;
    constexpr ImgSize& operator=(const ImgSize&) = default;

    const char* name;
    unsigned  x;
    unsigned  y;
    mutable char  buf[5] = { 0 };

    bool  operator==(const char* name_) const
    {
        return strcasecmp(name_, name) == 0;
    }

    const char* c_str() const
    {
	snprintf(buf, sizeof(buf), "%d", x);
	return buf;
    }

};

const std::array   stdImgSzs {
    ImgSize{  "web", 2048, 1536 },
    ImgSize{  "4mp", 2464, 1632 },
    ImgSize{  "6mp", 3008, 2000 },
    ImgSize{  "8mp", 3264, 2448 },
    ImgSize{ "12mp", 4320, 2868 },
    ImgSize{ "16mp", 4920, 3264 },
    ImgSize{ "24mp", 6048, 4024 }
};

const auto  MAX_THREAD_POOL_SIZE = std::thread::hardware_concurrency()*3;
const auto  DEFAULT_THREAD_POOL_SIZE = ceil(std::thread::hardware_concurrency()/2);

void  _usage(const char* argv0_)
{
		std::cout << argv0_ << " " << Imgcat::version() << "\n"
		     << "usage: " << argv0_ << " [ -p path ] [-c <target ICC profile location> | srgb] [-x] [-r] [-I] [-O <output size>] [-o JPEG | PNG | ORIG] [-q quality] [-R|-M] [-f]   file0 file1 .. fileN" << std::endl
		     << "  -p, --output-dir           extract preview images to location=./\n"
		     << "  -f, --force                overwrite existing preview images (default no)\n"
		     << "  -c, --colorspace           perform ICC conversion if possible: srgb for internal sRGB or file location of target ICC\n"
		     << "  -x, --exclude-meta         exclude metadata\n"
		     << "  -r, --reset-orientation    reset orientation (no orientation tag)\n"
		     << "  -I, --dump-icc             dump ICC to disk for each image\n"
		     << "  -O, --output-size          target (re)size\n"
		     << "  -o, --output-format        target output format\n"
		     << "  -q, --output-quality       quality\n"
		     << "  -R, --output-name-rand     random 16 byte hex for filename output (not incl extn)\n"
		     << "  -M, --output-name-meta     meta info used for filename output (not incl extn)\n"
		     << "  -T, --threads              concurrent extract (h/w=" << std::thread::hardware_concurrency() << " max=" << MAX_THREAD_POOL_SIZE << ")\n"
		     << "  -s, --strict               strict (warnings treated as errors, no attempted workarounds)\n"
		     << "  internal ICC profiles: ";

		const ICCprofiles*  p = theSRGBICCprofiles;
		while (p->profile) {
		    std::cout << " '" << p->name << "' (" << p->length << "bytes)";
		    ++p;
		}
		std::cout << std::endl;
exit(1);
}

int main(int argc, char* const argv[])
{
    const char*  argv0 = basename(argv[0]);

    const char*  thumbpath = "./";
    bool  dumpICC = false;
    bool  excludeMeta = false;
    unsigned  tpsz = DEFAULT_THREAD_POOL_SIZE;
    short convert = 0;
#define CONVERT_ICC 1
#define CONVERT_OUTPUT_FMT 2
#define CONVERT_QUALITY 4
#define CONVERT_RESIZE 8
#define CONVERT_RESET_ORIENTATION 16
    std::string  outputfmt;
    std::string  outputfmtExtn;

    const ICCprofiles*  tgtICC     = NULL;  // used to determine if ICC conversions req'd
    uchar_t*            nonSRGBicc = NULL;  // buf for non internal sRGB ICC
    std::unique_ptr<uchar_t[]>  mgr {nonSRGBicc};

    Magick::InitializeMagick(NULL);

    Magick::Geometry  target;
    enum FilenameType { FNT_FILE, FNT_RAND, FNT_META };
    FilenameType  fnt = FNT_FILE;
    int  imgqual = 100;
    std::list<std::future<void>>  futures;
    bool  overwrite = false;
    bool  strict = false;

    const struct option  long_opts[] = {
        { "output-dir",        1, 0, 'p' },
        { "force",             0, 0, 'f' },
        { "colorspace",        1, 0, 'c' },
        { "exclude-meta",      0, 0, 'x' },
        { "reset-orientation", 0, 0, 'r' },

        { "output-size",       1, 0, 'O' },
        { "output-format",     1, 0, 'o' },
        { "output-quality",    1, 0, 'q' },
        { "output-name-rand",  0, 0, 'R' },
        { "output-name-meta",  0, 0, 'M' },

        { "dump-icc",          0, 0, 'I' },

        { "threads",           1, 0, 'T' },
        { "strict",            0, 0, 's' },

        { "help",              0, 0, 'h' },
        { 0, 0, 0, 0 }
    };
    char  opt_args[1+ sizeof(long_opts)*2] = { 0 };
    {
        char*  og = opt_args;
        const struct option* op = long_opts;
        while (op->name) {
            *og++ = op->val;
            if (op->has_arg != no_argument) {
                *og++ = ':';
            }
            ++op;
        }
    }

    int  c;
    while ( (c=getopt_long(argc, argv, opt_args, long_opts, NULL)) != -1) {
	switch (c)
	{
	    case 'f':
		overwrite = true;
		break;

	    case 's':
		strict = true;
		break;

	    case 'p':
		thumbpath = optarg;
		break;

	    case 'q':
		imgqual = (unsigned)atol(optarg);
		if (imgqual > 100) {
		    imgqual = 100;
		}
		else {
		    convert |= CONVERT_QUALITY;
		}
		break;

	    case 'r':
	        convert |= CONVERT_RESET_ORIENTATION;
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
                if (tpsz > MAX_THREAD_POOL_SIZE) {
                    tpsz = MAX_THREAD_POOL_SIZE;
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
				nonSRGBicc = NULL;
			    }
			}
			close(fd);
		    }

		    if (tgtICC == NULL) {
			_usage(argv0);
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
		    const auto  stdsz = std::find(cbegin(stdImgSzs), cend(stdImgSzs), optarg);

		    const Magick::Geometry  g( (stdsz == std::end(stdImgSzs) ? optarg : stdsz->c_str()) );
		    const std::string  s = g;
		    target = s.c_str();
		    convert |= CONVERT_RESIZE;
		}
		catch (...)
		{ }
		break;

	    case 'o':
	    {
		auto  validOutputFmt = [&outputfmtExtn, &outputfmt](const char*) {
		    outputfmtExtn.clear();
		    if (strcasecmp(optarg, "JPEG") == 0 || strcasecmp(optarg, "JPG") == 0) {
			outputfmtExtn = ".jpg";
			outputfmt = "JPEG";

		    }

		    if (strcasecmp(optarg, "PNG") == 0) {
			outputfmtExtn = ".png";
			outputfmt = "PNG";
		    }

		    return !outputfmtExtn.empty();
		};

		if (validOutputFmt(optarg)) {
		    convert |= CONVERT_OUTPUT_FMT;
		}
		else {
		    if (strcasecmp(optarg, "DEFAULT") != 0) {
			std::cerr << argv0 << ": unsupported output format '" << optarg << "', will not convert output" << std::endl;
		    }
		}
	    } break;

	    case 'h':
	    default:
		_usage(argv0);
	}
    }
    mode_t  msk = umask(0);
    umask(msk);

    if (tpsz < 1) {
        tpsz = DEFAULT_THREAD_POOL_SIZE;
    }


    // make sure thumbpath is writable
    errno = 0;
    if (access(thumbpath, X_OK | W_OK) != 0)
    {
	if (errno == ENOENT) {
	    errno = 0;
	    mode_t  umsk = umask(0);
	    umask(umsk);
	    mkdir(thumbpath, 0777 & ~umsk);
	}


	if (errno != 0) {
	    std::cerr << argv0 << ": invalid thumbpath '" << thumbpath << "' - " << strerror(errno) << std::endl;
	    return 1;
	}
    }


    if (optind == argc) {
	std::cerr << argv0 << ": no input files" << std::endl;
	_usage(argv0);
    }

    if (tgtICC == NULL) {
	std::cout << argv0 << ": preview extract only (no ICC conversions)" << std::endl;
    }
    else
    {
	std::cout << argv0 << ": ICC conversions to " << (nonSRGBicc ? "external" : "internal") << " '" << tgtICC->name << "'";

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

    std::unique_ptr<Magick::Blob>  outicc { tgtICC ? new Magick::Blob(tgtICC->profile, tgtICC->length) : NULL };

    std::mutex  mtx;
    std::condition_variable  cond;
    unsigned  sem = tpsz;

    std::ofstream  devnull("/dev/null");
    std::streambuf*  erros = std::cerr.rdbuf(devnull.rdbuf());

    int  a = optind;
    const int  N = argc - a;
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
                auto  orig = Exiv2::ImageFactory::open(filename_);
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

                PreviewInfo  pvi(*(orig.get()));

                char  path[PATH_MAX];
                char  outputFilename[PATH_MAX];
                switch (fnt) {
                    case FNT_RAND:
                        strcpy(outputFilename, generate_hex(16).c_str());
                        break;

                    case FNT_META:
                    {
                        std::stringstream  d;
                        const char*  osep = pvi.seperator;
                        pvi.seperator = "-";
                        d << pvi;
                        strcpy(outputFilename, d.str().c_str());
                        pvi.seperator = osep;
                    } break;

                    default:
                        strcpy(outputFilename, filename_);
                }

		// strip off original extn
		char*  extpos = strrchr(outputFilename, '.');
		if (extpos) {
		    *extpos = '\0';
		}
                sprintf(path, "%s/%s", thumbpath, basename(outputFilename));

#define LOG_FILE_INFO  filename_ << ": " << std::setw(8) << prevp->size_ << " bytes, " << prevp->width_ << "x" << prevp->height_ << "  " << pvi

                std::cout << LOG_FILE_INFO << std::endl;

                Exiv2::PreviewImage  preview = loader.getPreviewImage(*prevp);

                auto  upd = Exiv2::ImageFactory::open( preview.pData(), preview.size() );

		// clear down metadata, some ImageMagick decoders bail if it encounters (valid) metadata it doesn't recognise, see libtiff:TIFFReadDirectory() for example - 
		upd->clearExifData();
                upd->clearIptcData();
                upd->clearXmpData();

                upd->writeMetadata();

                Exiv2::ExifData&  exif = orig->exifData();
                Exiv2::ExifData::iterator  iccavail = _extractICC(buf, exif);
		if (iccavail != exif.end() && dumpICC)
		{
		    char  iccpath[PATH_MAX];
		    sprintf(iccpath, "%s/%s.icc", thumbpath, basename(outputFilename));

		    if ( (fd = open(iccpath, O_CREAT | O_WRONLY, 0666 & ~msk)) < 0) {
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

                if (iccavail != exif.end() && tgtICC)
                {
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
                            std::cout << LOG_FILE_INFO << ": already sRGB (" << p->name << ") - not converting" << std::endl;
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
                        Magick::Image  img(Magick::Blob(upd->io().mmap(), upd->io().size()));

			if (convert & CONVERT_OUTPUT_FMT) {
			    img.magick(outputfmt);
			}

                        img.quality(imgqual);

			strcat(path,
			       convert & CONVERT_OUTPUT_FMT ?
			           outputfmtExtn.c_str() :
                                   preview.extension().c_str());

			if (access(path, F_OK) == 0 && !overwrite) {
			    err << LOG_FILE_INFO << ":  not overwritting existing output";
			    throw std::underflow_error(err.str());
			}

                        if (convert & CONVERT_ICC)
                        {
                            try
                            {
                                short  cretry = 5;
                                while (cretry--) {
                                    try
                                    {
                                        img.profile("ICC", Magick::Blob(buf.buf, buf.bufsz));
                                        img.profile("ICC", *outicc);
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
#ifdef HAVE_SAMPLE_ICC
				std::string  desc = "<unknown>", cprt;
				_extractICCinfo(buf.buf, buf.bufsz, desc, cprt);
#endif

				std::cout << LOG_FILE_INFO << ": ICC converted";
#ifdef HAVE_SAMPLE_ICC
				std::cout << " from " << desc;
#endif
				std::cout << std::endl;
                            }
                            catch (const Magick::Exception& ex)
                            {
                                std::cout << LOG_FILE_INFO << ": failed to convert ICC, ignoring - " << ex.what() << "\n";
                            }
                        }
                        // exif not maintained!!!

                        if (convert & CONVERT_RESIZE && target.isValid()) {
			    // figure out if need to flip bassed on img dimensions
			    const auto  h = img.baseRows();
			    const auto  w = img.columns();

			    Magick::Geometry  imgtarget;
			    std::string  x = "x";
			    if (h > w) {
			        imgtarget = x + (std::string)target;
			    }
			    else {
			        imgtarget = (std::string)target + x;
			    }
			    
                            img.resize(imgtarget);
                        }


			if (excludeMeta) {
			    img.profile("EXIF", Magick::Blob());
			    img.write(path);
			}
			else
			{

			    /* when a image format change is performed, exifdata is lost EVEN though
			     * we have set this via the `upd` object
			     *
			     * furthermore, exifProfile() returns an Blob that has 0 length; do it
			     * the dirty way throuh Exiv buffer
			     *
			     * if NO image format chagne is perfromed, the exif data is carried over
			     */
			    Magick::Blob  ci;
			    img.write(&ci);

			    auto  ce = Exiv2::ImageFactory::open( (Exiv2::byte*)ci.data(), ci.length() );

			    /* check portrait images - the original image may arleady have the orientation set, if we extract and copy of the exif, image viewers will have incorrect rotatation
			     */
			    ce->setMetadata(*orig.get());
			    if (convert & CONVERT_RESET_ORIENTATION) {
				auto&  cem = ce->exifData();
				cem["Exif.Image.Orientation"] = 1;  // std, top right orientation
			    }
			    ce->writeMetadata();
			    int  fd;
			    if ( (fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0666 & ~msk)) < 0) {
			        throw std::system_error(errno, std::system_category(), std::string{"failed to create "} + path);
			    }

			    if (write(fd, ce->io().mmap(), ce->io().size()) != ce->io().size()) {
			        close(fd);
			        throw std::system_error(errno, std::system_category(), std::string("failed to write ") + path);
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
                    if ( (fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0666 & ~msk)) < 0) {
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
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
            catch (const Exiv2::Error& e)
#else
            catch (const Exiv2::AnyError& e)
#endif
            {
                err << filename_ << ": unable to extract preview/reset exif - " << e;
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
    unsigned  success = 0;
    try
    {
        std::unique_lock<std::mutex>  lck(mtx);
        cond.wait(lck, [&sem, &tpsz]() { return sem == tpsz; });
        lck.unlock();
        std::cerr.rdbuf(erros);
        for (auto& f : futures) {
            try {
                f.get();
		++success;
            }
            catch (const std::exception& ex) {
                std::cerr << ex.what() << std::endl;
            }
        }
    }
    catch (const std::system_error& ex) {
        std::cerr << "failed to clean up threads - " << ex.what() << std::endl;
    }

    Magick::TerminateMagick();
    std::cout << "processed " << success << "/" << N << "\n";

    return success == N ? 0 : 1;
}
