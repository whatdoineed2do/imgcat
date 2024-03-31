#include "Ops.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <exception>


namespace ImgExtract {

void  OpWritePreview::execute()
{
    int  fd;
    if ( (fd = open(_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666 & ~_msk)) < 0) {
	_err << "failed to create preview - " << strerror(errno);
	throw std::system_error(errno, std::system_category(), _err.str());
    }

    Exiv2::BasicIo&  rawio = _exiv->io();
    rawio.seek(0, Exiv2::BasicIo::beg);

    if (write(fd, rawio.mmap(), rawio.size()) != rawio.size()) {
	close(fd);
	_err << "failed to write converted preview - " << strerror(errno);
	throw std::system_error(errno, std::system_category(), _err.str());
    }
    close(fd);
}

void  OpExcludeMeta::execute()
{
}

void  OpCopyMeta::execute()
{
};

void  OpResize::execute()
{
};

void  OpConvertFmt::execute()
{
};


void  OpConvertICC::execute()
{
}


}
