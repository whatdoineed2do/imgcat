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

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <string>
#include <list>
#include <filesystem>

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

    Istat(const std::string& filename_, const struct stat& st_) : filename(filename_)
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

using Istats = std::list<Istat>;


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
	       const char* where_, const char** extn_, const char** vextn_)
{
    const std::filesystem::path  where(where_);
    for (auto const&  de : std::filesystem::recursive_directory_iterator{where}) 
    {
	if (!de.is_regular_file()) {
	    continue;
	}

	struct stat  st;
	stat(de.path().c_str(), &st);

	if (_filterextn(extn_, de.path().c_str())) {
	    files_.emplace_back(Istat(de.path(), st));
	}
	else if (_filterextn(vextn_, de.path().c_str())) {
	    vfiles_.emplace_back(Istat(de.path(), st));
	}
    }
}

void  _readdir(const ImgMetaParser& metaparser_, ImgIdx& idx_, const char* thumbpath_, const char* where_, const char**  extn_)
{
    const std::filesystem::path  where(where_);
    for (auto const&  de : std::filesystem::recursive_directory_iterator{where}) 
    {
	if (de.is_regular_file()) {
	    continue;
	}

	struct stat  st;
	stat(de.path().c_str(), &st);
	DLOG(path);
	if (_filterextn(extn_, de.path().c_str())) {
	    const Img  img = metaparser_.parse(de.path().c_str(), st, thumbpath_);
	    idx_[img.key].emplace_back(std::move(img.data));
	}
    }
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
using Tasks = std::list<_Task*>;



int main(int argc, char **argv)
{
    const char* DFLT_EXTNS = ".jpg;.jpeg;.nef;.tiff;.tif;.png;.gif;.cr2;.raf;.dng;.heic;.heif";
    const char* DFLT_VEXTNS = ".mov;.mp4;.avi;.mpg;.mpeg";
    const char*  extns  = DFLT_EXTNS;
    const char*  vextns = DFLT_VEXTNS;

#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,27,4)
    Exiv2::enableBMFF();
#endif

    const char*  thumbpath = ".thumbs";
    unsigned  thumbsize = 640;
    char*  p = NULL;

    bool  verbosetime = false;
    const unsigned  MAX_THREAD_POOL_SIZE = std::thread::hardware_concurrency()*3;
    unsigned  tpsz = std::thread::hardware_concurrency();
    bool  generate = true;

    ImgOut*  outgen = NULL;

    const std::chrono::time_point<std::chrono::system_clock>  start = std::chrono::system_clock::now();

    const struct option  long_opts[] = {
        { "image-extns", 1, 0, 'I' },
        { "video-extns", 1, 0, 'V' },
        { "no-thumbs",   0, 0, 'n' },
        { "thumb-path",  1, 0, 't' },
        { "thumb-size",  1, 0, 's' },
        { "output",      1, 0, 'H' },

        { "threads",     1, 0, 'T' },
        { "verbose",     0, 0, 'v' },
        { "help",        0, 0, 'h' },
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
    while ( (c=getopt_long(argc, argv, opt_args, long_opts, NULL)) != -1)
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
		if (tpsz > MAX_THREAD_POOL_SIZE) {
		    tpsz = MAX_THREAD_POOL_SIZE;
		}
	    } break;

	    case 's':
	    {
		thumbsize = (unsigned)atol(optarg);
	    } break;

	    case 'v':
	    {
		verbosetime = true;
	    } break;

            case 'H':
            {
                if ( (outgen = ImgOut::create(optarg)) == NULL && strcasecmp(optarg, "help") == 0) {
		    return 1;
                }
            } break;

	    case 'n':
            {
                generate = false;
            } break;

	    case 'h':
	    usage:
	    default:
		const char*  argv0 = basename(argv[0]);

		std::cout << argv0 << " " << Imgcat::version() << "\n\n"
		          << "usage: " << argv0 << "[OPTIONS] <dir0, dir1 ..>\n"
			  << "  -I, --image-extns <extensions>    image extns: " << DFLT_EXTNS << "\n"
			  << "  -V, --video-extns <extensions>    video extns: " << DFLT_VEXTNS << "\n"
			  << "  -n, --no-thumbs                   do NOT generate thumbs\n"
			  << "  -t. --thumb-path <path>           location of generated thumbnails\n"
			  << "  -s, --thumb-size <size>           generated thumbnail size: " << thumbsize << "\n"
			  << "  -T, --threads <thread pool size>  max threads=" << tpsz << "\n"
			  << "  -H, --output <generator>          try 'help' for output type\n"
                     << "\n"
                     << "use MAGICK_TMPDIR= to are suitably free disk if default /tmp or /var/tmp dirs get full" << std::endl;
		return 1;
	}
    }

    if (outgen == nullptr) {
        outgen = ImgOut::create(NULL);  // ask for the default
    }
    if (tpsz < 1) {
        tpsz = 1;
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
	    unsigned  n = 0;

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
		memset(*target, 0, sizeof(char*)*(n+1));
		char**  eptr = *target;

		char*  pc = NULL;
		char*  tok = NULL;
		while ( (tok = strtok_r(pc == NULL ? e : NULL, ".;", &pc)) )
		{
		    const unsigned  n = strlen(tok);
		    *eptr++ = strcpy(new char[n+1], tok);
		}
	    }
	    delete []  e;
	}
    }

    if (optind == argc) {
        std::cerr << "no directories to index\n";
        goto usage;
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
    unsigned  ttlfiles = 0;
    std::cout << "initial dir scan" << std::endl;
    while (optind < argc)
    {
	char*  dir = argv[optind++];
	const unsigned  dirlen = strlen(dir);
	if ( dir[dirlen-1] == '/' ) {
	    dir[dirlen-1] = (char)NULL;
	}
	std::cout << "  working on " << dir << "  " << std::flush;
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
		    std::cout << '#' << std::flush;
		}
		catch (const std::invalid_argument& ex)
		{
		    ignored.emplace_back(i.filename, ex.what());
		}
	    }

	    for (auto&  i : vidfilenames)
	    {
		try
		{
		    const Img  img = avfmtparser.parse(i.filename.c_str(), i.st, thumbpath);
		    idxs.back()[img.key].emplace_back(std::move(img.data));
		    std::cout << '%' << std::flush;
		}
		catch (const std::exception& ex)
		{
		    DLOG("failed to video parse - " << ex.what());
		    ignored.emplace_back(i.filename, ex.what());
		}
	    }
	    ttlfiles += imgfilenames.size() + vidfilenames.size();

	    std::cout << std::endl;
	    if (verbosetime) {
		gettimeofday(&tvC, NULL);
		std::cout << "  " << dir << " -> " << ttlfiles << " files, (img=" << imgfilenames.size() << " vid=" << vidfilenames.size() << ") read=" << (double)(tvB - tvA)/1000000 << ", parse=" << (double)(tvC - tvB)/1000000 << std::endl;
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

        auto thumbgener = [&generate](const ImgIdx::Ent& imgidx_, unsigned thumbsize_) {
            return generate ? new ImgThumbGen(imgidx_, thumbsize_) :
                              new ImgThumbNoGen(imgidx_, thumbsize_);
        };

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
		tasks.push_back(new _Task(thumbgener(j, thumbsize), mtx, cond, --tpsz) );

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
