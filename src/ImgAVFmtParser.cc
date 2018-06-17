#include "ImgAVFmtParser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <iostream>

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

    {
	std::ostringstream  tmp;
	tmp << thumbpath_ << "/" << st_.st_dev << "-" << st_.st_ino;
	imgdata.thumb = std::move(tmp.str());
    }

#if 1
    AVFormatContext*  avfmt   = NULL;
    AVCodecContext*   avcodec = NULL;

    int ret;
    if ((ret = avformat_open_input(&avfmt, filename_, NULL, NULL)) != 0) {
	av_strerror(ret, _averr, sizeof(_averr));

	std::ostringstream  err;
	err << "failed avformat_open_input - " << filename_ << " - " << _averr;
	throw std::invalid_argument(err.str());
    }

#if 0
    while ((tag = av_dict_get(avfmt->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
	std::cout << "  " << tag->key << "=" << tag->value << std::endl;
    }
#endif

    imgdata.metavid.duration = avfmt->duration/AV_TIME_BASE;

    avformat_find_stream_info(avfmt, NULL);
    int  vidstream = -1;
    for (int i=0; i< avfmt->nb_streams; i++) {
	if (avfmt->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
	    vidstream = i;
	    break;
	}
    }
    if (vidstream < 0) {
	avformat_close_input(&avfmt);
	std::ostringstream  err;
	err << "failed avformat_open_input - " << filename_ << " - " << _averr;
	throw std::underflow_error(err.str());
    }

    avcodec = avfmt->streams[vidstream]->codec;
    {
	std::ostringstream  dim;
	dim << avcodec->width << "x" << avcodec->height;
	imgdata.xy = std::move(dim.str());
    }

    imgdata.metavid.framerate = av_q2d(avcodec->framerate);

    struct TagsOI {
	//TagsOI(const char* tag_, const std::string& s_) : tag(tag_), s(s_) { }

        const char*   tag;
	std::string&  s;
    }
    toi[] = {
	//{ "major_brand",   imgdata.metavid.container },
	{ "model",         imgdata.metavid.model },
	{ "date",          imgdata.moddate },
	{ "creation_time", imgdata.moddate },
	{ NULL, _tmp }
    };

    const TagsOI*  pt = toi;
    while (pt->tag)
    {
	AVDictionaryEntry*  avtag = NULL;
	if ( (avtag = av_dict_get(avfmt->metadata, pt->tag, avtag, AV_DICT_IGNORE_SUFFIX)) ) {
	    if (avtag->value && pt->s.empty()) {
		pt->s = avtag->value;
	    }
	}

	++pt;
    }

    avformat_close_input(&avfmt);
#endif
    time_t  t = st_.st_mtime;
    if (!imgdata.moddate.empty()) {
	// should be ISO8601 - "2013-06-19T14:46:34.000000Z" or 2013-07-18T09:11:58+0100
	char tmp[20];
	strncpy(tmp, imgdata.moddate.c_str(), 19);
	tmp[19] = NULL;
        struct tm  tm;
	memset(&tm, 0, sizeof(tm));
	if ( strptime(tmp, "%Y-%m-%dT%T", &tm)) {
	    t = mktime(&tm);
	}
    }

    return Img(ImgKey(st_.st_ino, t), imgdata);
}
