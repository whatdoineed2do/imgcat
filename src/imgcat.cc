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


#include "Img.h"
#include "ImgIdx.h"
#include "ImgExifParser.h"
#include "ImgAVFmtParser.h"
#include "ImgOut.h"
#include "ImgHtml.h"
#include "ImgThumbGen.h"
#include "version.h"



#ifdef DEBUG_LOG
#define DLOG(x)  std::cout << "DEBUG:  " << x << std::endl;
#else
#define DLOG(x)
#endif


struct Istat {
    std::string  filename;
    struct stat  st;

    Istat(const char* filename_, const struct stat& st_) : filename(filename_)
    {
	memcpy(&st, &st_, sizeof(st));
    }

    Istat(const Istat&& rhs_) : filename(std::move(rhs_.filename))
    {
	memcpy(&st, &rhs_.st, sizeof(st));
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

    const Istat& operator=(const Istat&& rhs_)
    {
	if (this != &rhs_) {
	    filename = std::move(rhs_.filename);
	    memcpy(&st, &rhs_.st, sizeof(st));
	}
	return *this;
    }
};

typedef std::list<Istat>  Istats;


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


void  _readdir(Istats& files_, Istats& vfiles_,
	       const char* where_, const char** extn_, const char** vextn_)  throw (std::invalid_argument)
{
    DIR*  d;
    if ( (d = opendir(where_)) == NULL) {
	std::ostringstream  err;
	err << "failed to open '" << where_ << "' - " << strerror(errno);
	throw std::invalid_argument(err.str());
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

	    std::ostringstream  err;
	    err << "failed to stat '" << path << "' - " << strerror(errno);
	    throw std::invalid_argument(err.str());
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
			files_.emplace_back(Istat(path, st));
		    }
		    else if (_filterextn(vextn_, path)) {
			vfiles_.emplace_back(Istat(path, st));
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

void  _readdir(const ImgMetaParser& metaparser_, ImgIdx& idx_, const char* thumbpath_, const char* where_, const char**  extn_)  throw (std::invalid_argument)
{
    DIR*  d;
    if ( (d = opendir(where_)) == NULL) {
	std::ostringstream  err;
	err << "failed to open '" << where_ << "' - " << strerror(errno);
	throw std::invalid_argument(err.str());
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

	    std::ostringstream  err;
	    err << "failed to stat '" << path << "' - " << strerror(errno);
	    throw std::invalid_argument(err.str());
	}

	try
	{
	    if (st.st_mode & S_IFDIR) {
		_readdir(metaparser_, idx_, thumbpath_, path, extn_);
	    }
	    else
	    {
		DLOG(path);
		if (st.st_mode & S_IFREG && _filterextn(extn_, path)) {
		    const Img  img = metaparser_.parse(path, st, thumbpath_);
		    idx_[img.key].emplace_back(std::move(img.data));
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



struct _Task 
{
    std::future<void>  f;  // used to determe task complete
    ImgThumbGen*  task;

    // external locks for semaphore
    std::mutex&  _mtx;
    std::condition_variable&  _cond;
    unsigned&  _sem;

    // internal locks
    std::mutex  _m;
    std::condition_variable  _c;

    bool  _b;



    _Task(ImgThumbGen* task_, std::mutex& mtx_, std::condition_variable& cond_, unsigned& sem_)
        : task(task_), _mtx(mtx_), _cond(cond_), _sem(sem_),
	  _b(false)
    {
        f = std::async(std::launch::async, &_Task::run, this);
    }


    // mtx/conds are not movable
    _Task(_Task&&) = delete;
    _Task&  operator=(const _Task&)  = delete;
    _Task&  operator=(const _Task&&) = delete;

    void run()
    {
	std::unique_lock<std::mutex>  lck(_m);
	if (_b) {
	    return;
	}

	try
	{
	    task->generate();
	}
	catch (const std::exception& ex)
	{ }

	_mtx.lock();
	++_sem;
	_cond.notify_all();
	_mtx.unlock();

	_b = true;
	_c.notify_all();
	lck.unlock();
    }

    ImgThumbGen*  release()
    {
        ImgThumbGen*  byebye = task;

	std::unique_lock<std::mutex>  lck(_m);
        bool&  b = _b;
        _c.wait(lck, [&b]{ return b; });
        task = NULL;
        lck.unlock();

        return byebye;
    }

    ~_Task()
    { delete task; }
};
typedef std::list<_Task*>  Tasks;



int main(int argc, char **argv)
{
    const char* DFLT_EXTNS = ".jpg;.jpeg;.nef;.tiff;.tif;.png;.gif;.cr2;.raf";
    const char* DFLT_VEXTNS = ".mov;.mp4;.avi;.mpg;.mpeg";
    const char*  extns  = DFLT_EXTNS;
    const char*  vextns = DFLT_VEXTNS;

    const char*  thumbpath = ".thumbs";
    unsigned  thumbsize = 640;
    int  width = 8;
    char*  p = NULL;

    bool  verbosetime = false;
    unsigned  tpsz = 8;

    ImgOut*  outgen = NULL;

    const std::chrono::time_point<std::chrono::system_clock>  start = std::chrono::system_clock::now();

    int  c;
    while ( (c = getopt(argc, argv, "I:V:t:T:s:w:H:hv")) != EOF)
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

            case 'H':
            {
                if ( (outgen = ImgOut::create(optarg)) == NULL) {
                }
            } break;

	    case 'h':
	    usage:
	    default:
		const char*  argv0 = basename(argv[0]);

		std::cout << argv0 << " " << Imgcat::version() << "\n\n"
		          << "usage: " << argv0 << "\n"
			  << "           [-I " << DFLT_EXTNS << "]\t\timage extns\n"
			  << "           [-V " << DFLT_VEXTNS << "]\t\tvideo extns\n"
			  << "           [-t <thumbpath=.thumbs>]\t\tlocation of thumbpath\n"
			  << "           [-s <thumbsize=150>]\t\tgenerated thumb size\n"
			  << "           [-T <max threads=" << tpsz << ">]\t\tthread pool size\n"
			  << "           [-H <html output, try 'help']\t\tHTML output\n"
			  << "           <dir0> <dir1> <...>" << std::endl
                     << "\n"
                     << "use MAGICK_TMPDIR= to are suitably free disk if default /tmp or /var/tmp dirs get full" << std::endl;
		return 1;
	}
    }

    if (outgen == nullptr) {
        outgen = ImgOut::create(NULL);  // ask for the default
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
		std::cout << "considering all files" << std::endl;
	    }
	    else
	    {
		DLOG(src << "  n=" << n << " tokens");

		*target = new char*[n+1];
		memset(*target, 0, n+1); // BUG?
		char**  eptr = *target;

		char*  pc = NULL;
		char*  tok = NULL;
		while ( (tok = strtok_r(pc == NULL ? e : NULL, ".;", &pc)) )
		{
		    const uint_t  n = strlen(tok);
		    *eptr++ = strcpy(new char[n+1], tok);
		}
                *eptr++ = NULL;
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
		std::cerr << "invalid thumbpath '" << thumbpath << "' - " << strerror(errno) << std::endl;
		goto usage;
	    }
	}
    }


    ImgIdxs  idxs;
    bool  allok = true;
    struct _Ignored {
	const std::string  file;
	const std::string  reason;

	_Ignored(const std::string& file_, std::string&& reason_) : file(file_), reason(std::move(reason_)) { }
	_Ignored(const std::string& file_, const char* reason_) : file(file_), reason(reason_) { }
	_Ignored(_Ignored& rhs_) : file(rhs_.file), reason(rhs_.reason) { }
	_Ignored(_Ignored&& rhs_) : file(std::move(rhs_.file)), reason(std::move(rhs_.reason)) { }

	_Ignored& operator=(_Ignored&) = delete;
	_Ignored& operator=(_Ignored&&) = delete;
    };

    ImgExifParser   exifparser;
    ImgAVFmtParser  avfmtparser;

    std::list<_Ignored>  ignored;
    uint_t  ttlfiles = 0;
    while (optind < argc)
    {
	char*  dir = argv[optind++];
	const uint_t  dirlen = strlen(dir);
	if ( dir[dirlen-1] == '/' ) {
	    dir[dirlen-1] = (char)NULL;
	}
	idxs.emplace_back(ImgIdx(dir));

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
	    for (const auto&  i : imgfilenames)
	    {
		try
		{
		    const Img  img = exifparser.parse(i.filename.c_str(), i.st, thumbpath);
		    idxs.back()[img.key].emplace_back(std::move(img.data));
		}
		catch (const std::invalid_argument& ex)
		{
		    ignored.emplace_back(i.filename, ex.what());
		}
	    }

	    for (Istats::iterator i=vidfilenames.begin(); i!=vidfilenames.end(); ++i)
	    for (auto&  i : vidfilenames)
	    {
		try
		{
		    const Img  img = avfmtparser.parse(i.filename.c_str(), i.st, thumbpath);
		    idxs.back()[img.key].emplace_back(std::move(img.data));
		}
		catch (const std::exception& ex)
		{
		    DLOG("failed to video parse - " << ex.what());
		    ignored.emplace_back(i.filename, ex.what());
		}
	    }
	    ttlfiles += imgfilenames.size() + vidfilenames.size();

	    if (verbosetime) {
		gettimeofday(&tvC, NULL);
		std::cout << dir << " -> " << imgfilenames.size() << " files, " << " read=" << (double)(tvB - tvA)/1000000 << ", parse=" << (double)(tvC - tvB)/1000000 << std::endl;
	    }
	}
	catch (const std::exception& ex)
	{
	    std::cerr << "failed to process all entries in '" << dir << "' - " << ex.what() << std::endl;
	    allok = false;
	    break;
	}
    }
    std::cout << "scanned " << ttlfiles << " imgs from " << idxs.size() << " dirs" << std::endl;

    if (allok)
    {
        std::mutex  mtx;
        std::condition_variable  cond;

        ImgThumbGens  imgthumbs;
        ImgOut::Payloads  htmlpayloads;

	std::cout << "generating thumbnail previews.." << std::endl;
	for (auto&  idx : idxs)
	{
	    std::cout << "  working on [" << std::setw(3) << idx.size() << "]  " << idx.id << "  " << std::flush;

	    if (idx.empty()) {
		std::cout << '\n';
		continue;
	    }

	    idx.sort();


	    Tasks  tasks;
	    for (const auto&  j : idx)
	    {
                // wait for allowable thread to be available
                std::unique_lock<std::mutex>  lck(mtx);
                cond.wait(lck, [&tpsz]{ return tpsz > 0;});

		/* grab the exif and thumb from the very first item which is
		 * supposed to be the primary image
		 */
		tasks.push_back(new _Task(new ImgThumbGen(j, thumbsize), mtx, cond, --tpsz) );

                lck.unlock();
		std::cout << "#" << std::flush;
	    }


            htmlpayloads.emplace_back(ImgOut::Payload(idx));

	    for (auto&  t : tasks)
	    {
		t->f.get();

		if ( !t->task->error().empty() ) {
		    std::cerr << t->task->error() << std::endl;
		}

                /* this set of thumbs is for this idx, need to handoff otherwise
                 * the imgthumbs contains ALL the thumbs for all dirs
                 */
                ImgThumbGen*  itg = t->release();
                htmlpayloads.back().thumbs.push_back(itg);
                imgthumbs.push_back(itg);
		delete t;
	    }
	    std::cout << std::endl;
	}
    	
	if (!ignored.empty()) {
	    std::cout << "ignored " << ignored.size() << " files:\n";
	    for (const auto& i : ignored) {
		std::cout << "  " << i.file << " - " << i.reason << std::endl;
	    }
	}

        std::cout << "generating output" << std::endl;
        {
            mode_t  umsk = umask(0);
            umask(umsk);

            // generate the html tbl
            const std::string  out = outgen->generate(htmlpayloads);
            if (out.size() == 0) {
                std::cerr << "generated output is empty!" << std::endl;
		allok = false;
            }
            else
            {
                int  fd;
                if ( (fd = open(outgen->filename().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666 & ~umsk)) < 0) {
                    std::cerr << "failed to create " << outgen->filename() << " - " << strerror(errno) << " - will use stdout" << std::endl;
                    std::cout << out << std::endl;
		    allok = false;
                }
                else
                {
                    if ( write(fd, out.c_str(), out.size()) != out.size()) {
                        std::cerr << "failed to write all data - " << strerror(errno) << std::endl;
                        std::cout << out << std::endl;
			allok = false;
                    }
                    close(fd);
                }
            }
        }

        for (auto th : imgthumbs) {
            delete th;
        }
        imgthumbs.clear();
    }


    const std::chrono::time_point<std::chrono::system_clock>  now = std::chrono::system_clock::now();
    const std::chrono::duration<double>  elapsed = now - start;
    std::cout << "completed in " << elapsed.count() << " secs" << std::endl;

    idxs.clear();
    delete outgen;

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
