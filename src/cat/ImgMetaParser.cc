#include "ImgMetaParser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

#include <sstream>


const Img  ImgMetaParser::parse(const char* filename_, const struct stat& st_, const char* thumbpath_) const
{
    if (access(filename_, R_OK) < 0) {
	std::ostringstream  err;
	err << strerror(errno);
	throw std::invalid_argument(err.str());
    }

    return _parse(filename_, st_, thumbpath_);
}
