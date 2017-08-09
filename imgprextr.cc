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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <limits.h>
#include <libgen.h>

#include <string>
#include <iostream>
#include <cassert>

using namespace  std;

#include <exiv2/exiv2.hpp>
#include <Magick++.h>
#ifdef HAVE_SAMPLE_ICC
#include <IccProfile.h>
#include <IccTag.h>
#include <IccUtil.h>
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


class _Buf
{
  public:
    _Buf() : buf(NULL), sz(0), bufsz(0) { }
    _Buf(size_t sz_) : buf(NULL), sz(0), bufsz(0) { alloc(sz_); }

    ~_Buf() { free(); }

    uchar_t*  buf;
    size_t    bufsz;
    size_t    sz;

    void  alloc(size_t sz_)
    {
	if (sz_ > sz) {
	    delete [] buf;
	    sz = sz_;
	    bufsz = sz;
	    buf = new uchar_t[sz];
	}
	memset(buf, 0, sz);
    }

    void  free()
    {
	delete []  buf;
	buf = NULL;
	sz = 0;
	bufsz = 0;
    }

    const uchar_t*  copy(uchar_t* buf_, size_t sz_)
    {
	alloc(sz_);
	memcpy(buf, buf_, sz_);
	bufsz = sz_;
	return buf;
    }

    void  clear()
    {
	memset(buf, 0, sz);
    }

  private:
    _Buf(const _Buf&);
    void operator=(const _Buf&);
};



Exiv2::ExifData::iterator  _extractICC(_Buf& buf_, Exiv2::ExifData& exif_)
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
		string  orig;
		string  mod;

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
	cout << "data size = " << datasz_ << endl;
	cout << "profile size = " << profile.length() << endl;
	if (profile.length() == 0) {
#if 0
	    // ??? lets be trying ICC/ICM
	    const Magick::Blob   icc  = magick.profile("ICC");
	    const Magick::Blob   icm  = magick.profile("ICM");

	    cout << "  explicit icc len=" << icc.length() << endl
	         << "  explicit icm len=" << icm.length() << endl;

	    mode_t  msk = umask(0);
	    umask(msk);
	    int  fd;
	    if ( (fd = open("icc.icc", O_CREAT | O_TRUNC, 0666 & ~msk)) < 0) {
		cerr << "  failed to create icc profile - " << strerror(errno) << endl;
	    }
	    else 
	    {
		if ( write(fd, icc.data(), icc.length()) != profile.length()) {
		    cerr << "  failed to write profile data - " << strerror(errno) << endl;
		}
	    }
	    close(fd);

	    if ( (fd = open("icm.icc", O_CREAT | O_TRUNC, 0666 & ~msk)) < 0) {
		cerr << "  failed to create icm profile - " << strerror(errno) << endl;
	    }
	    else 
	    {
		if ( write(fd, icm.data(), icm.length()) != profile.length()) {
		    cerr << "  failed to write profile data - " << strerror(errno) << endl;
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
	    cerr << "  failed to create profile - " << strerror(errno) << endl;
	    return;
	}

	if ( write(fd, profile.data(), profile.length()) != profile.length()) {
	    cerr << "  failed to write profile data - " << strerror(errno) << endl;
	}
	close(fd);
    }
    catch (const std::exception& ex)
    {
	cerr << "failed to magick - " << ex.what() << endl;
    }
}

string  _extraInfo(const Exiv2::Image& img_)
{
    string  mftr;
    string  camera;
    string  sn;

    typedef Exiv2::ExifData::const_iterator (*EasyAccessFct)(const Exiv2::ExifData& ed);
    static const struct _EasyAccess {
	EasyAccessFct find;
	string*       target;
    } eatags[] = {
	{ Exiv2::make,         &mftr   },
	{ Exiv2::model,        &camera },
	{ Exiv2::serialNumber, &sn     },
	{ NULL, NULL }
    };

    const Exiv2::ExifData&  ed = img_.exifData();
    if (ed.empty()) {
	return "";
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

    if (mftr.length() > 10) {
	mftr.replace(10, mftr.length(), 3, '.');
    }

    ostringstream  dump;
    dump << mftr << " " << camera << " " << sn;
    return dump.str();
}


bool  _extractICCinfo(const void* data_, const size_t datasz_, string& desc_, string& cprt_)
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

int main(int argc, char* const argv[])
{
    const char*  argv0 = basename(argv[0]);

    const char*  thumbpath = "./";
    bool  dumpICC = false;
    bool  excludeMeta = false;

    const ICCprofiles*  tgtICC     = NULL;  // used to determine if ICC conversions req'd
    uchar_t*            nonSRGBicc = NULL;  // buf for non internal sRGB ICC

    Magick::Geometry  target;

    char c;
    while ( (c=getopt(argc, argv, "p:Ic:xhO:")) != EOF) {
	switch (c)
	{
	    case 'p':
		thumbpath = optarg;
		break;

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
			    cerr << argv0 << ": invalid ICC " << "'" << optarg << "' - " << strerror(errno) << endl;
			    return 1;
			}

			if (st.st_size == 0 || S_ISREG(st.st_mode) == 0) {
			    cerr << argv0 << ": invalid ICC " << "'" << optarg << "'" << endl;
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
				cerr << argv0 << ": unable to read ICC '" << optarg << "' - " << strerror(errno) << endl;
				delete [] nonSRGBicc;
				nonSRGBicc = NULL;
			    }
			}
			close(fd);
		    }

		    if (tgtICC == NULL) {
			goto usage;
		    }
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
		    const string  s = g;
		    target = s.c_str();
		}
		catch (...)
		{ }
		break;

	    case 'h':
	    default:
usage:
		cout << "usage: " << argv0 << " [ -p path ] [-c <target ICC profile>] [-x] [-I] [-O <output size>]   file0 file1 .. fileN" << endl
		     << "         -p    extract preview images to location=./" << endl
		     << "         -c    perform ICC conversion to sRGB if possible" << endl
		     << "         -x    exclude metadata" << endl
		     << "         -I    dump ICC to disk for each image" << endl
		     << "         -O    target (re)size" << endl
		     << "  internal ICC profiles: ";

		const ICCprofiles*  p = theSRGBICCprofiles;
		while (p->profile) {
		    cout << " '" << p->name << "' (" << p->length << "bytes)";
		    ++p;
		}
		cout << endl;
		return 1;
	}
    }
    mode_t  msk = umask(0);
    umask(msk);


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
	    cerr << argv0 << ": invalid thumbpath '" << thumbpath << "' - " << strerror(errno) << endl;
	    return 1;
	}
    }


    if (optind == argc) {
	cerr << argv0 << ": no input files" << endl;
	goto usage;
    }

    if (tgtICC == NULL) {
	cout << argv0 << ": preview extract only (no ICC conversions)" << endl;
    }
    else
    {
	cout << argv0 << ": ICC target=" << (nonSRGBicc ? "external" : "internal") << " '" << tgtICC->name << "'";

#ifdef HAVE_SAMPLE_ICC
	string  tgtICCdesc = "<no internal desc>";
	string  tgtICCcprt = "<no internal copyright>";

	if ( !_extractICCinfo(tgtICC->profile, tgtICC->length, tgtICCdesc, tgtICCcprt)) {
	    cerr << "  invalid ICC fmt" << endl;
	    return -1;
	}
	cout << "  " << tgtICCdesc << " (c) " << tgtICCcprt << endl;
#else
	cout << endl;
#endif
    }

    _Buf  buf(1024);
    const Magick::Blob*  outicc = tgtICC ? new Magick::Blob(tgtICC->profile, tgtICC->length) : NULL;
    int  fd;

    int  a = optind;
    while (a < argc)
    {
	const char* const  filename = argv[a++];
	try
	{
	    Exiv2::Image::AutoPtr  orig = Exiv2::ImageFactory::open(filename);
	    assert(orig.get() != 0);


	    orig->readMetadata();

	    Exiv2::PreviewManager loader(*orig);
	    Exiv2::PreviewPropertiesList  list = loader.getPreviewProperties();

	    // grabbing the largest preview
	    Exiv2::PreviewPropertiesList::iterator prevp = list.begin();
	    if (prevp == list.end()) {
		cout << filename << ":  no preview" << endl;
		continue;
	    }

	    advance(prevp, list.size()-1);

	    char  path[PATH_MAX];
	    char  path1[PATH_MAX];
	    strcpy(path1, filename);
	    sprintf(path, "%s/%s", thumbpath, basename(path1));

#define LOG_FILE_INFO  filename << ": " << setw(8) << prevp->size_ << " bytes, " << prevp->width_ << "x" << prevp->height_ << "  " << _extraInfo(*orig)

	    cout << LOG_FILE_INFO << endl;

	    Exiv2::PreviewImage  preview = loader.getPreviewImage(*prevp);
	    strcat(path, preview.extension().c_str());

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
	    bool  dumporig = true;

	    if (iccavail != exif.end() && tgtICC)
	    {
		if (dumpICC)
		{
		    sprintf(path, "%s/%s.icc", thumbpath, basename(path1));

#ifdef __MINGW32__
		    if ( (fd = open(path, O_CREAT | O_WRONLY | O_BINARY, 0666 & ~msk)) < 0) {
#else
		    if ( (fd = open(path, O_CREAT | O_WRONLY, 0666 & ~msk)) < 0) {
#endif
			cerr << argv0 << ": " << LOG_FILE_INFO << ": failed to create ICC - " << strerror(errno) << endl;
		    }
		    else
		    {
			if (write(fd, buf.buf, buf.bufsz) != buf.bufsz) {
			    cerr << argv0 << ": " << LOG_FILE_INFO << ": failed to write ICC - " << strerror(errno) << endl;
			}
			close(fd);
		    }
		}

		bool  convert = true;
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
			cout << argv0 << ": " << LOG_FILE_INFO << ": already sRGB (" << p->name << ") - not converting" << endl;
			convert = false;
		    }
		    else
		    {
			/* the ICC profile is different to the any of the known
			 * sRGBs so we'll have to convert
			 */
		    }
		}


		if (convert)
		{
		    try
		    {
			if (!excludeMeta) {
			    exif.erase(iccavail);
			    upd->setByteOrder(orig->byteOrder());
			    upd->setExifData(exif);
			    upd->writeMetadata();
			}

			/* grab hold of underlying and updated data */
			Exiv2::BasicIo&  rawio = upd->io();
			rawio.seek(0, Exiv2::BasicIo::beg);

			Magick::Image  img(Magick::Blob(rawio.mmap(), rawio.size()));

			img.quality(100);
			img.profile("ICC", Magick::Blob(buf.buf, buf.bufsz));
			img.profile("ICC", *outicc);
			img.iccColorProfile(*outicc);

#ifdef HAVE_SAMPLE_ICC
			string  desc, cprt;
			_extractICCinfo(buf.buf, buf.bufsz, desc, cprt);
#endif

			cout << argv0 << ": " << LOG_FILE_INFO << ": ICC converted";
#ifdef HAVE_SAMPLE_ICC
			cout << " from " << desc;
#endif
			cout << endl;
			if (target.isValid()) {
				img.resize(target);
			}
			img.write(path);

			dumporig = false;
		    }
		    catch (const std::exception& ex)
		    {
			cout << argv0 << ": " << LOG_FILE_INFO << ": failed to write (sRGB converted) preview - " << ex.what() << endl;
		    }
		}
	    }


	    if (dumporig)
	    {
#ifdef __MINGW32__
		if ( (fd = open(path, O_CREAT | O_WRONLY | O_BINARY, 0666 & ~msk)) < 0) {
#else
		if ( (fd = open(path, O_CREAT | O_WRONLY, 0666 & ~msk)) < 0) {
#endif
		    cerr << argv0 << ": " << LOG_FILE_INFO << ": failed to create preview - " << strerror(errno) << endl;
		    continue;
		}

		Exiv2::BasicIo&  rawio = upd->io();
		rawio.seek(0, Exiv2::BasicIo::beg);

		if (write(fd, rawio.mmap(), rawio.size()) != rawio.size()) {
		    cerr << argv0 << ": " << LOG_FILE_INFO << ": failed to write preview - " << strerror(errno) << endl;
		}
		close(fd);
	    }
	}
	catch (const Exiv2::AnyError& e)
	{
	    cout << filename << ":  unable to extract preview/reset exif - " << e << endl;
	    continue;
	}
    }

    delete [] nonSRGBicc;
    delete outicc;

    return 0;
}
