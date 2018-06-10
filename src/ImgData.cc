#include "ImgData.h"
#include <iostream>
#include <iomanip>

std::ostream&  operator<<(std::ostream& os_, const ImgData& obj_)
{
    struct Pf {
	const std::string& s;
	const char* pre;
	const char* suf;
    }
    pf[] = {
	{ obj_.metaimg.camera,     NULL, NULL },
	{ obj_.metaimg.sn,         NULL, NULL },
	{ obj_.metaimg.shuttercnt, "(#", ")" },
	{ obj_.metaimg.lens,       NULL, NULL },
	{ obj_.metaimg.focallen,   NULL, NULL },
	{ obj_.metaimg.prog,       NULL, NULL },
	{ obj_.metaimg.iso,        NULL, "ISO" },
	{ obj_.metaimg.shutter,    NULL, NULL },
	{ obj_.metaimg.aperture,   NULL, NULL },
	{ obj_.metaimg.wb,         NULL, " WB" },
	//{ obj_.dpi, NULL, "dpi" }

        { obj_.metavid.container, NULL, NULL },
	{ obj_.metavid.model,     NULL, NULL }
    };


    for (short i=0; i<sizeof(pf)/sizeof(struct Pf); ++i) {
	if (pf[i].s.empty()) {
	    continue;
	}
	os_ << (pf[i].pre ? pf[i].pre : "") << pf[i].s << (pf[i].suf ? pf[i].suf : "") << "  ";
    }

    if (obj_.metavid.duration > 0) {
	const auto&  d = obj_.metavid.duration;

	short  secs = d%60;
	short  mins = d/60;
	int    hrs = 0;
	if (d  > 3599) {
	    hrs = d/3600;
	    mins = d%hrs;
	    os_ << std::setfill('0') << std::setw(2) <<  hrs << ':';
	}
	os_ << std::setfill('0') << std::setw(2) << mins << ':'
	    << std::setfill('0') << std::setw(2) << secs << "  ";
    }
    if (obj_.metavid.framerate > 0) {
	os_ << obj_.metavid.framerate << "fps  ";
    }
    return os_;
}

