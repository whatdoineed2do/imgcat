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


#include "Img.h"
#include "ImgIdx.h"
#include "ImgExifParser.h"
#include "ImgHtml.h"
#include "ImgThumbGen.h"



#ifdef DEBUG_LOG
#define DLOG(x)  cout << "DEBUG:  " << x << endl;
#else
#define DLOG(x)
#endif


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



struct _Task 
{
    std::future<void>  f;  // used to determe task complete
    ImgThumbGen*  task;

    std::mutex&  _mtx;
    std::condition_variable&  _cond;
    unsigned*  _sem;


    _Task(ImgThumbGen* task_, std::mutex& mtx_, std::condition_variable& cond_, unsigned& sem_)
        : task(task_), _mtx(mtx_), _cond(cond_), _sem(&sem_)
    {
        f = std::async(std::launch::async, &_Task::run, this);
    }

    void run()
    {
        if (_sem == NULL || task == NULL) {
            return;
        }


        if (task )
        {
            task->generate();

            _mtx.lock();
            ++(*_sem);
            _mtx.unlock();
            _cond.notify_all();

            _sem = NULL;
        }
    }

    ImgThumbGen*  release()
    {
        ImgThumbGen*  byebye = task;
        // TODO - theres a potential race here
        if (_sem) {
            _mtx.lock();
            ++(*_sem);
            _mtx.unlock();
            _cond.notify_all();
        }
        task = NULL;
        return byebye;
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
    unsigned  thumbsize = 300;
    int  width = 8;
    char*  p = NULL;

    bool  verbosetime = false;
    unsigned  tpsz = 8;

    ImgHtml*  htmlgen = NULL;

    const chrono::time_point<std::chrono::system_clock>  start = std::chrono::system_clock::now();

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
                if ( (htmlgen = ImgHtml::create(optarg)) == NULL) {
                }
            } break;

	    case 'h':
	    usage:
	    default:
		cout << "usage: " << argv[0] << " [-I " << DFLT_EXTNS << " -V " << DFLT_VEXTNS << " ]  [-t <thumbpath=.thumbs>]  [-s <thumbsize=150>]  [-w <imgs per row=8>] [-T <max threads=8>] <dir0> <dir1> <...>" << endl;
		return 1;
	}
    }

    if (htmlgen == NULL) {
        htmlgen = ImgHtml::create(NULL);  // ask for the default
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
        std::mutex  mtx;
        std::condition_variable  cond;

        ImgThumbGens  imgthumbs;
        ImgHtml::Payloads  htmlpayloads;

	cout << "generating thumbnail previews.." << endl;
	for (ImgIdxs::const_iterator i=idxs.begin(); i!=idxs.end(); ++i) 
	{
	    ImgIdx&  idx = **i;
	    cout << "  working on [" << setw(3) << idx.size() << "]  " << idx.id << "  " << flush;

	    if (idx.empty()) {
		cout << '\n';
		continue;
	    }

	    idx.sort();


	    Tasks  tasks;
	    for (ImgIdx::const_iterator j=idx.begin(); j!=idx.end(); ++j)
	    {
                // wait for allowable thread to be available
                std::unique_lock<std::mutex>  lck(mtx);
                cond.wait(lck, [&tpsz]{ return tpsz > 0;});

		/* grab the exif and thumb from the very first item which is
		 * supposed to be the primary image
		 */
		tasks.push_back( new _Task(new ImgThumbGen(*j, thumbsize), mtx, cond, --tpsz) );

                lck.unlock();
		cout << "#" << flush;
	    }


            htmlpayloads.push_back(ImgHtml::Payload(idx));

	    for (Tasks::const_iterator t=tasks.begin(); t!=tasks.end(); ++t)
	    {
		(*t)->f.get();

		if ( !(*t)->task->error().empty() ) {
		    cerr << (*t)->task->error() << endl;
		}

                /* this set of thumbs is for this idx, need to handoff otherwise
                 * the imgthumbs contains ALL the thumbs for all dirs
                 */
                ImgThumbGen*  itg = (*t)->release();
                htmlpayloads.back().thumbs.push_back(itg);
                imgthumbs.push_back(itg);
                delete *t;
	    }
	    cout << endl;
	}
    	
	if (!ignored.empty()) {
	    cout << "ignored " << ignored.size() << " images:\n";
	    for (const auto i : ignored) {
		cout << "  " << i << endl;
	    }
	}

        cout << "generating html output" << endl;
        {
            mode_t  umsk = umask(0);
            umask(umsk);

            // generate the html tbl
            const std::string  out = htmlgen->generate(htmlpayloads);
            if (out.size() == 0) {
                cerr << "generated output is empty!" << endl;
            }
            else
            {

                int  fd;
                if ( (fd = open("index.html", O_WRONLY | O_CREAT | O_TRUNC, 0666 & ~umsk)) < 0) {
                    cerr << "failed to create index.html - " << strerror(errno) << " - will use stdout" << endl;
                    cout << out << endl;
                }
                else
                {
                    if ( write(fd, out.c_str(), out.size()) != out.size()) {
                        cerr << "failed to write all data to index.html - " << strerror(errno) << endl;
                        cout << out << endl;
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


    const chrono::time_point<std::chrono::system_clock>  now = std::chrono::system_clock::now();
    const chrono::duration<double>  elapsed = now - start;
    cout << "completed in " << elapsed.count() << " secs" << endl;

    for (auto i : idxs) {
	delete i;
    }
    idxs.clear();
    delete htmlgen;

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
