#include "ImgData.h"
#include <iostream>

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
	{ obj_.metaimg.wb,         NULL, " WB" }
	//{ obj_.dpi, NULL, "dpi" }
    };

    for (short i=0; i<sizeof(pf)/sizeof(struct Pf); ++i) {
	if (pf[i].s.empty()) {
	    continue;
	}
	os_ << (pf[i].pre ? pf[i].pre : "") << pf[i].s << (pf[i].suf ? pf[i].suf : "") << "  ";
    }
    return os_;
}

