#include "ImgAVFmtParser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
}


bool  ImgAVFmtParser::_initd = false;

ImgAVFmtParser::ImgAVFmtParser()
{
    if (!ImgAVFmtParser::_initd) {
	av_register_all();
	ImgAVFmtParser::_initd = true;
    }
}


const Img  ImgAVFmtParser::_parse(const char* filename_, const struct stat& st_, const char* thumbpath_) const
     throw (std::invalid_argument, std::range_error, std::underflow_error, std::overflow_error)
{
    ImgData  imgdata(filename_, st_.st_size);
    imgdata.type = ImgData::VIDEO;
    ostringstream  tmp;
    tmp << thumbpath_ << "/" << st_.st_dev << "-" << st_.st_ino;
    imgdata.thumb = tmp.str();
    return Img(ImgKey(st_.st_ino, st_.st_mtime), imgdata);
}
