#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <strings.h>
#include <getopt.h>
#include <errno.h>

typedef unsigned int  uint_t;
typedef long long  longlong_t;


#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <string>
#include <list>

#include <chrono>
#include <mutex>
#include <future>
using namespace  std;

#include <exiv2/exiv2.hpp>
#include <Magick++.h>

#include <libffmpegthumbnailer/videothumbnailer.h>
#include <libffmpegthumbnailer/filmstripfilter.h>


#include "Img.h"
#include "ImgIdx.h"
#include "ImgExifParser.h"

#include "ICCprofiles.h"



#ifdef DEBUG_LOG
#define DLOG(x)  cout << "DEBUG:  " << x << endl;
#else
#define DLOG(x)
#endif


typedef list<ImgIdx*>  ImgIdxs;

longlong_t  operator-(const timeval& lhs_, const timeval& rhs_)
{
    const longlong_t  x = lhs_.tv_sec*1000000 + lhs_.tv_usec;
    const longlong_t  y = rhs_.tv_sec*1000000 + rhs_.tv_usec;
    return x - y;
}

struct Istat {
    string  filename;
    struct stat  st;

    Istat(const char* filename_, const struct stat& st_) : filename(filename_)
    {
	memcpy(&st, &st_, sizeof(st));
    }

    Istat(const Istat& rhs_) : filename(rhs_.filename)
    {
	memcpy(&st, &rhs_.st, sizeof(st));
    }

    const Istat& operator=(const Istat& rhs_)
    {
	if (this != &rhs_) {
	    filename = rhs_.filename;
	    memcpy(&st, &rhs_.st, sizeof(st));
	}
	return *this;
    }
};

typedef list<Istat>  Istats;
typedef list<string>  strings;


bool  _filterextn(const char** extn_, const char* path_)
{
    if (extn_ == NULL) {
	return true;
    }

    // pull the last part of filename to see if we care
    // about these file types
    const char*  pf = strrchr(path_, '.');
    if (pf == NULL || ++pf == NULL) {
	DLOG("ignoring " << path_ << " - not matching extension");
	return false;
    }
    const int  n = strlen(pf);

    const char**  pe = extn_;
    while (*pe) {
	DLOG(*pe << " == " << pf);
	if (strncasecmp(*pe, pf, n) == 0) {
	    break;
	}
	++pe;
    }

    if (*pe == NULL) {
	DLOG("ignoring " << path_ << " - no matching extension");
	return false;
    }
    return true;
}


void  _readdir(Istats& files_, Istats& vfiles_, const char* where_, const char** extn_, const char** vextn_)  throw (invalid_argument)
{
    DIR*  d;
    if ( (d = opendir(where_)) == NULL) {
	ostringstream  err;
	err << "failed to open '" << where_ << "' - " << strerror(errno);
	throw invalid_argument(err.str());
    }

    char  path[PATH_MAX];

    const size_t  dsz = (sizeof(struct dirent) + pathconf(where_, _PC_NAME_MAX));
    struct dirent*  dent = (struct dirent*)malloc(dsz);
    memset(dent, 0, dsz);
    struct dirent*  result = NULL;
    while ( readdir_r(d, dent, &result) == 0 && result)
    {
	if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0 || strcmp(result->d_name, "Thumbs.db") == 0) {
	    continue;
	}

	struct stat  st;
	sprintf(path, "%s/%s", where_, result->d_name);
	if (stat(path, &st) < 0)
	{
	    free(dent);

	    ostringstream  err;
	    err << "failed to stat '" << path << "' - " << strerror(errno);
	    throw invalid_argument(err.str());
	}

	try
	{
	    if (st.st_mode & S_IFDIR) {
		_readdir(files_, vfiles_, path, extn_, vextn_);
	    }
	    else {
		DLOG(path);
		if (st.st_mode & S_IFREG)
		{
		    if (_filterextn(extn_, path)) {
			files_.push_back(Istat(path, st));
		    }
		    else if (_filterextn(vextn_, path)) {
			vfiles_.push_back(Istat(path, st));
		    }
		}
	    }
	}
	catch (...)
	{
	    free(dent);
	    throw;
	}
    }
    free(dent);
    closedir(d);
}

void  _readdir(ImgIdx& idx_, const char* thumbpath_, const char* where_, const char**  extn_)  throw (invalid_argument)
{
    DIR*  d;
    if ( (d = opendir(where_)) == NULL) {
	ostringstream  err;
	err << "failed to open '" << where_ << "' - " << strerror(errno);
	throw invalid_argument(err.str());
    }

    char  path[PATH_MAX];

    struct dirent*  dent = (struct dirent*)malloc(sizeof(struct dirent) + pathconf(where_, _PC_NAME_MAX));
    struct dirent*  result = NULL;
    while ( readdir_r(d, dent, &result) == 0 && result)
    {
	if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") || strcmp(result->d_name, "Thumbs.db") == 0) {
	    continue;
	}

	struct stat  st;
	sprintf(path, "%s/%s", where_, result->d_name);
	if (stat(path, &st) < 0)
	{
	    free(dent);

	    ostringstream  err;
	    err << "failed to stat '" << path << "' - " << strerror(errno);
	    throw invalid_argument(err.str());
	}

	try
	{
	    if (st.st_mode & S_IFDIR) {
		_readdir(idx_, thumbpath_, path, extn_);
	    }
	    else
	    {
		DLOG(path);
		if (st.st_mode & S_IFREG && _filterextn(extn_, path)) {
		    const Img  img = ImgExifParser::parse(path, st, thumbpath_);
		    idx_[img.key].push_back(img.data);
		}
	    }
	}
	catch (...)
	{
	    free(dent);
	    throw;
	}
    }
    free(dent);
    closedir(d);
}




void  _genthumbnail(const string& path_, const string& origpath_, Magick::Image& img_, const unsigned sz_, const float rotate_)
{
    try
    {
	if (rotate_) {
	    img_.rotate(rotate_);
	}

	/* make sure that the thumb is at least 'sz_'
	 * wide
	 */
	ostringstream  tmp;
	if (img_.rows() < img_.columns()) {
	    tmp << "x" << sz_;
	}
	else {
	    tmp << sz_ << "x";
	}
	img_.scale(tmp.str().c_str());

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
	img_.write(path_.c_str());
    }
    catch (const std::exception& ex)
    {
	ostringstream  err;
	err << "failed to resize thumbnail " << origpath_ << " - " << ex.what();
	throw range_error(err.str());
    }
}

void  _genthumbnail(const string& path_, const string& origpath_, const void* data_, const size_t datasz_, const unsigned sz_, const float rotate_)
{
    try
    {
	DLOG("thumbnail path=" << path_ << " orig=" << origpath_);
	Magick::Blob   blob(data_, datasz_);
	Magick::Image  magick(blob);

	_genthumbnail(path_, origpath_, magick, sz_, rotate_);
    }
    catch (const std::exception& ex)
    {
	ostringstream  err;
	err << "failed to resize thumbnail, unable to create internal obj " << origpath_ << " - " << ex.what();
	throw range_error(err.str());
    }
}


void  _readgenthumbnail(const ImgData& img_, const string& prevpath_, const unsigned sz_)
{
    int  fd;
    if ( (fd = open(img_.filename.c_str(), O_RDONLY)) < 0) {
	cerr << "failed to open source file to generate thumbnail: " << img_.filename << " - " << strerror(errno) << endl;
    }
    else
    {
	errno = 0;
	char*  buf = new char[img_.size];
	size_t  n;
	if ( (n = read(fd, buf, img_.size)) != img_.size) {
	    cerr << "failed to read source file to generate thumbnail: " << img_.filename << " (read " << n << "/" << img_.size << ") - " << strerror(errno) << endl;
	}
	else {
	    _genthumbnail(prevpath_, img_.filename, buf, img_.size, sz_, img_.rotate);
	}
	delete []  buf;
	close(fd);
    }
}



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

void  _genthumbnail(const string& path_, const string& origpath_, const Exiv2::PreviewImage& preview_, const Exiv2::ExifData& exif_, const unsigned sz_, const float rotate_)
{
    const char*  iccproftag = "Exif.Image.InterColorProfile";

    static _Buf  buf(1024);

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
	    DLOG(origpath_ << "  colourspace=" << d->toLong());
	    if (d->toLong() == 2)
	    {
		// it was shot as aRGB - check the orig vs mod times
		string  orig;
		string  mod;

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
	_genthumbnail(path_, origpath_, preview_.pData(), preview_.size(), sz_, rotate_);
    }
    else
    {
	DLOG(origpath_ << "      not sRGB");

	static  const Magick::Blob  outicc(theSRGBICCprofiles[0].profile, theSRGBICCprofiles[0].length);

	Magick::Image  img(Magick::Blob( preview_.pData(), preview_.size() ));
	img.profile("ICC", Magick::Blob(buf.buf, buf.bufsz));
	img.profile("ICC", outicc);

	_genthumbnail(path_, origpath_, img, sz_, rotate_);
    }
}

class _TNGen
{
  public:
    _TNGen(const ImgIdx::Ent& imgidx_, unsigned thumbsize_, std::mutex& mtx_, std::condition_variable& cond_, unsigned& sem_)  throw ()
	: _imgidx(imgidx_), _img(*_imgidx.imgs.begin()), _prevpath(_img.thumb + ".jpg"),
	  _thumbsize(thumbsize_), _completed(false),
          _mtx(mtx_), _cond(cond_), _sem(sem_)
    { }

    ~_TNGen()
    { }

    _TNGen(const _TNGen&) = delete;
    _TNGen& operator=(const _TNGen&) = delete;

    _TNGen(_TNGen&& rhs_)
    	: _imgidx(rhs_._imgidx), _img(rhs_._img), _prevpath(std::move(rhs_._prevpath)),
	  _thumbsize(rhs_._thumbsize), _completed(rhs_._completed),
          _mtx(rhs_._mtx), _cond(rhs_._cond), _sem(rhs_._sem)
    { }

    _TNGen& operator=(_TNGen&& rhs_) = delete;


    void  run()
    {
	//cout << "/" << flush;
	DLOG("running " << _img);

	gettimeofday(&_x, NULL);
	const ImgData&  img = _img;

	/* grab the exif and thumb from the very first item which is
	 * supposed to be the primary image
	 */

	// explicity want a jpg
	const string  prevpath = _prevpath;
	try
	{
	    if (img.type == ImgData::EMBD_PREVIEW)
	    {
		/* this is most likely a raw file which has embedded thumbs
		 */
		try
		{
		    Exiv2::Image::AutoPtr  orig = Exiv2::ImageFactory::open(img.filename);
		    orig->readMetadata();
		    Exiv2::PreviewManager  prevldr(*orig);
		    Exiv2::PreviewPropertiesList  prevs = prevldr.getPreviewProperties();
		    if (prevs.empty()) {
			//cout << "no preview - " << img.filename << endl;
			_readgenthumbnail(img, prevpath, _thumbsize);
		    }
		    else
		    {
			// get the largest, convert to sRGB if possible and scale
			const Exiv2::PreviewImage  preview = prevldr.getPreviewImage(prevs.back());

			_genthumbnail(prevpath, img.filename, preview, orig->exifData(), _thumbsize, img.rotate);
		    }
		}
		catch (const Exiv2::AnyError& e)
		{
		    _error << "unable to extract thumbnail from " << img.filename << " - " << e;
		}
	    }
	    else
	    {
		if (img.type == ImgData::IMAGE)
		{
		    _readgenthumbnail(img, prevpath, _thumbsize);
		}
		else if (img.type == ImgData::VIDEO)
		{
		    ffmpegthumbnailer::VideoThumbnailer videoThumbnailer(_thumbsize, true, true, 10, false);
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
	catch (const exception& e)
	{
	    _error << e.what();
	}

	gettimeofday(&_y, NULL);
	_completed = true;
	//cout << "\\" << flush;

        _mtx.lock();
        ++_sem;
        _mtx.unlock();
        _cond.notify_all();
    }


    const ImgIdx::Ent&  idx() const
    {
	return _imgidx;
    }

    const ImgData&  img() const
    {
	return _img;
    }

    const string  prevpath() const
    {
	return _prevpath;
    }


    bool  completed() const
    {
	return _completed;
    }

    double  ms() const
    {
	return (double)(_y - _x)/1000000;
    }


    const string  error() const
    { return _error.str(); }


  private:

    bool  _completed;

    const ImgIdx::Ent&  _imgidx;
    const ImgData&  _img;
    const string  _prevpath;

    const unsigned  _thumbsize;

    struct timeval  _x, _y;

    ostringstream  _error;

    std::mutex&  _mtx;
    std::condition_variable&  _cond;
    unsigned&  _sem;
};

struct _Task {
    std::future<void>  f;  // used to determe task complete
    _TNGen*  task;

    _Task(_TNGen* task_) : task(task_)
    {
        f = std::async(std::launch::async, &_TNGen::run, task);
    }

    ~_Task()
    { delete task; }
};
typedef list<_Task*>  Tasks;



int main(int argc, char **argv)
{
    const char* DFLT_EXTNS = ".jpg;.jpeg;.nef;.tiff;.tif;.png;.gif;.cr2;.raf";
    const char* DFLT_VEXTNS = ".mov;.mp4;.avi;.mpg;.mpeg";
    const char*  extns  = DFLT_EXTNS;
    const char*  vextns = DFLT_VEXTNS;

    const char*  thumbpath = ".thumbs";
    unsigned  thumbsize = 150;
    int  width = 8;
    char*  p = NULL;

    bool  verbosetime = false;
    unsigned  tpsz = 8;

    const chrono::time_point<std::chrono::system_clock>  start = std::chrono::system_clock::now();

    int  c;
    while ( (c = getopt(argc, argv, "I:V:t:T:s:w:hv")) != EOF)
    {
	switch (c)
	{
	    case 'I':
	    {
		extns = (strlen(optarg) == 0) ? NULL : optarg;
	    } break;

	    case 'V':
	    {
		vextns = (strlen(optarg) == 0) ? NULL : optarg;
	    } break;

	    case 't':
	    {
		thumbpath = strcmp(optarg, "NULL") == 0 ? NULL : optarg;
	    } break;

	    case 'T':
	    {
		tpsz = (unsigned)atol(optarg);
		if (tpsz > 128) {
		    tpsz = 128;
		}
	    } break;

	    case 's':
	    {
		thumbsize = (unsigned)atol(optarg);
	    } break;

	    case 'w':
	    {
		width = (short)atoi(optarg);
	    } break;

	    case 'v':
	    {
		verbosetime = true;
	    } break;

	    case 'h':
	    usage:
	    default:
		cout << "usage: " << argv[0] << " [-I " << DFLT_EXTNS << " -V " << DFLT_VEXTNS << " ]  [-t <thumbpath=.thumbs>]  [-s <thumbsize=150>]  [-w <imgs per row=8>] [-T <max threads=8>] <dir0> <dir1> <...>" << endl;
		return 1;
	}
    }



    // go and validate/split the extns we care about
    char**  extn  = NULL;
    char**  vextn = NULL;

    for (int i=0; i<2; ++i)
    {
	char***  target;
	const char*   src;
	if (i == 0) {
	    target = &extn;
	    src = extns;
	}
	else
	{
	    target = &vextn;
	    src = vextns;
	}

	if (src)
	{
	    char*  e = strcpy(new char[strlen(extns)+1], src);
	    uint_t  n = 0;

	    const char*  e1 = e;
	    while (*e1) {
		if (*e1++ == '.') {
		    ++n;
		}
	    }

	    if (n == 0) {
		cout << "considering all files" << endl;
	    }
	    else
	    {
		DLOG(src << "  n=" << n << " tokens");

		*target = new char*[n+1];
		memset(*target, 0, n+1);
		char**  eptr = *target;

		char*  pc = NULL;
		char*  tok = NULL;
		while ( (tok = strtok_r(pc == NULL ? e : NULL, ".;", &pc)) )
		{
		    const uint_t  n = strlen(tok);
		    *eptr++ = strcpy(new char[n+1], tok);
		}
	    }
	    delete []  e;
	}
    }



    if (thumbpath) {
	mode_t  msk = umask(0);
	umask(msk);
	if (mkdir(thumbpath, 0777 & ~msk) < 0)
	{
	    if (errno == EEXIST && access(thumbpath, W_OK) == 0) {
	    }
	    else {
		cerr << "invalid thumbpath '" << thumbpath << "' - " << strerror(errno) << endl;
		goto usage;
	    }
	}
    }


    ImgIdxs  idxs;
    bool  allok = true;
    strings  ignored;
    uint_t  ttlfiles = 0;
    while (optind < argc)
    {
	char*  dir = argv[optind++];
	const uint_t  dirlen = strlen(dir);
	if ( dir[dirlen-1] == '/' ) {
	    dir[dirlen-1] = (char)NULL;
	}
	idxs.push_back(new ImgIdx(dir));

	try
	{
	    Istats  imgfilenames;
	    Istats  vidfilenames;

	    struct timeval  tvA, tvB, tvC;

	    if (verbosetime) {
		gettimeofday(&tvA, NULL);
	    }
	    _readdir(imgfilenames, vidfilenames, dir, (const char**)extn, (const char**)vextn);
	    if (verbosetime) {
		gettimeofday(&tvB, NULL);
	    }
	    for (Istats::const_iterator i=imgfilenames.begin(); i!=imgfilenames.end(); ++i)
	    {
		try
		{
		    const Img  img = ImgExifParser::parse(i->filename.c_str(), i->st, thumbpath);
		    (*idxs.back())[img.key].push_back(img.data);
		}
		catch (const invalid_argument& ex)
		{
		    ignored.push_back(i->filename);
		}
	    }

	    for (Istats::iterator i=vidfilenames.begin(); i!=vidfilenames.end(); ++i)
	    {
		try
		{
		    ImgData  imgdata(i->filename.c_str(), i->st.st_size);
		    imgdata.type = ImgData::VIDEO;
		    ostringstream  tmp;
		    tmp << thumbpath << "/" << i->st.st_dev << "-" << i->st.st_ino;
		    imgdata.thumb = tmp.str();
		    Img  img(ImgKey(i->st.st_ino, i->st.st_mtime), imgdata);
		    (*idxs.back())[img.key].push_back(imgdata);
		}
		catch (const exception& ex)
		{
		    DLOG("failed to parse - " << ex.what());
		    ignored.push_back(i->filename);
		}
	    }
	    ttlfiles += imgfilenames.size();

	    if (verbosetime) {
		gettimeofday(&tvC, NULL);
		cout << dir << " -> " << imgfilenames.size() << " files, " << " read=" << (double)(tvB - tvA)/1000000 << ", parse=" << (double)(tvC - tvB)/1000000 << endl;
	    }
	}
	catch (const exception& ex)
	{
	    cerr << "failed to process all entries in '" << dir << "' - " << ex.what() << endl;
	    allok = false;
	    break;
	}
    }
    cout << "scanned " << ttlfiles << " imgs from " << idxs.size() << " dirs" << endl;

    if (allok)
    {
	mode_t  umsk = umask(0);
	umask(umsk);

	int  fd;
	if ( (fd = open("index.html", O_WRONLY | O_CREAT | O_TRUNC, 0666 & ~umsk)) < 0) {
	    cerr << "failed to create index.html - " << strerror(errno) << " - will use stdout" << endl;
	}

	// generate the html tbl
	ostringstream  html;

        std::mutex  mtx;
        std::condition_variable  cond;

	cout << "generating index.html and thumbnail previews.." << endl;
	for (ImgIdxs::const_iterator i=idxs.begin(); i!=idxs.end(); ++i) 
	{
	    ImgIdx&  idx = **i;
	    cout << "  working on [" << setw(3) << idx.size() << "]  " << idx.id << "  " << flush;
	    html << "<h2>/<a href=\"" << idx.id << "\">" << idx.id << "</a></h2>";

	    if (idx.empty()) {
		cout << '\n';
		continue;
	    }

	    idx.sort();

	    int  tj = 0;
	    int  tk = 0;

	    ImgIdx::const_iterator  dts = idx.begin();

	    html << "<p><sup>" << dts->key.dt.hms;

	    advance(dts, idx.size()-1);
	    if (dts != idx.end()) {
		html << " .. " << dts->key.dt.hms;
	    }
	    html << "</sup></p>"
	         << "<table cellpadding=0 cellspacing=2 frame=border>";

	    Tasks  tasks;
	    for (ImgIdx::const_iterator j=idx.begin(); j!=idx.end(); ++j)
	    {
#if 0
		cout << j->key << " => " << j->imgs.size() << endl;
		for (ImgIdx::Imgs::const_iterator k=j->imgs.begin(); k!=j->imgs.end(); ++k) {
		    cout << "  " << *k << " => " << k->thumb << endl;
		}
#endif


                // wait for allowable thread to be available
                std::unique_lock<std::mutex>  lck(mtx);
                cond.wait(lck, [&tpsz]{ return tpsz > 0;});

		_TNGen*  task = new _TNGen(*j, thumbsize, mtx, cond, --tpsz);
                lck.unlock();


		/* grab the exif and thumb from the very first item which is
		 * supposed to be the primary image
		 */
		tasks.push_back( new _Task(task) );
		cout << "#" << flush;
	    }


	    for (Tasks::const_iterator t=tasks.begin(); t!=tasks.end(); ++t)
	    {
		(*t)->f.get();

		const ImgData&  img = (*t)->task->img();
		if ( !(*t)->task->error().empty() ) {
		    cerr << (*t)->task->error() << endl;
		}

		if (tk == 0) {
		     html << "<tr height=" << thumbsize << " align=center valign=middle>";
		}

		// insert the caption for the cell
		html << "<td width=" << thumbsize 
		     << " title=\"";
		if (!img.rating.empty()) {
		    html << "[" << img.rating << "] ";
		}
		html << img.title << " (" << (*t)->task->idx().key.dt.hms << ") Exif: " << img << "\">"
		     << "<a href=\"" << img.filename << "\"><img src=\"" << (*t)->task->prevpath() << "\"></a>";

		ImgIdx::Imgs::const_iterator  alts = (*t)->task->idx().imgs.begin();
		while (alts != (*t)->task->idx().imgs.end()) {
		    const ImgData&  altimg = *alts++;
		    html << "<a href=\"" << altimg.filename << "\">. </a>";
		}
		html << "</td>";
		    
		if (++tk == width) {
		    html << "</tr>\n";
		    tk = 0;
		}
		cout << '.' << flush;
		if (verbosetime) {
		    cout << (*t)->task->ms();
		}

		delete *t;
	    }
	    if (tk) {
		html << "</tr>";
	    }
	    html << "</table>\n\n";
	    cout << endl;
	}

	const string  out = html.str();
	if (fd) {
	    if ( write(fd, out.c_str(), out.size()) != out.size()) {
		cerr << "failed to write all data to index.html - " << strerror(errno) << endl;
		allok = false;
	    }
	    close(fd);
	}
	else {
	    cout << out << endl;
	}
	
	if (!ignored.empty()) {
	    cout << "ignored " << ignored.size() << " images:\n";
	    for (strings::const_iterator i=ignored.begin(); i!=ignored.end(); ++i) {
		cout << "  " << *i << endl;
	    }
	}
    }
    const chrono::time_point<std::chrono::system_clock>  now = std::chrono::system_clock::now();
    const chrono::duration<double>  elapsed = now - start;
    cout << "completed in " << elapsed.count() << " secs" << endl;

    for (auto i : idxs) {
	delete i;
    }
    idxs.clear();

    char**  pp = extn;
    while (*pp) {
	delete [] *pp++;
    }
    pp = vextn;
    while (*pp) {
	delete [] *pp++;
    }
    delete []  extn;
    delete []  vextn;

    return allok ? 0 : 1;
}
