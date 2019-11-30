#include "ImgOutJson.h"

#include <iostream>
#include <iomanip>

std::string  ImgOutJsonJS::filename()
{
    return std::string("data.js");
}

std::string  ImgOutJsonJS::generate(ImgOut::Payloads& payloads_)
{
    std::ostringstream  body;
    body << "var jsondata = '" << _impl.generate(payloads_) << "';";
    return std::move(body.str());
}

std::string  ImgOutJson::filename()
{
    return std::string("data.json");
}


std::string  ImgOutJson::generate(ImgOut::Payloads& payloads_)
{
    unsigned  imgttl = 0;
    std::string  datestart;
    std::string  dateend;
    std::ostringstream  body;
    body << "{ \"data\": [";

    ImgIdx::Stats  ttlstats;
    auto  _genJsonStat = [&ttlstats](std::ostringstream& body, const ImgIdx::Stats& stats)
    {
	struct P {
	    const char*  category;
	    const ImgIdx::Stats::_Stat&  stat;
	}
	all[] = {
	    { "cameras",    stats.camera },
	    { "lenses",     stats.lens },
	    { "focal_lengths", stats.focallen },
	    { NULL,         stats.camera },
	};
	P*  p = all;
	while (p->category) {
	    body << ", \"" << p->category << "\": [";
	    bool i = true;
	    for (const auto&  si : p->stat) {
		body << (i ? ' ' : ',') << "{\"id\": \"" << (si.first.empty() ? "n/a" : si.first.c_str()) << "\", \"count\":" << si.second << "}";
		i = false;
	    }
	    ++p;
	    body << "]";
	}
    };


    unsigned  pblck = 0;
    for (auto& p : payloads_)
    {
        ImgIdx&        idx    = p.idx;
        ImgThumbGens&  thumbs = p.thumbs;
        imgttl += thumbs.size();
        
        std::cout << "  working on [" << std::setw(3) << idx.size() << "]  " << idx.id << "  " << std::flush;

        const auto  pos = idx.id.rfind('/');
        body << (pblck++ == 0 ? ' ' : ',') << " { \"id\": \"" << (pos == std::string::npos ? idx.id : idx.id.substr(pos+1)) << "\", \"path\": \"" << idx.id << "\", \"count\": " << thumbs.size();

        idx.sort();
        for (ImgIdx::const_iterator  dtsf=idx.begin(); dtsf!=idx.end(); ++dtsf)
        {
            if (dtsf->key.dt.hms.empty()) {
                continue;
            }
            body << ", \"date_start\": \"" << dtsf->key.dt.hms << '"';
	    if (datestart.empty() || datestart > dtsf->key.dt.hms) {
		datestart = dtsf->key.dt.hms;
	    }

            for (ImgIdx::const_reverse_iterator  dts=idx.crbegin(); dts!=idx.crend(); ++dts) {
                if (dts->key.dt.hms.empty()) {
                    continue;
                }

                body << ", \"date_end\": \"" << dts->key.dt.hms << '"';
		if (dateend.empty() || dateend < dts->key.dt.hms) {
		    dateend = dts->key.dt.hms;
		}
                break;
            }

            break;
        }
	const auto& stats = idx.stats();
	_genJsonStat(body, stats);
	ttlstats += stats;

        body << ", \"images\": ["; 
        bool j = true;
        for (auto& t : thumbs)
        {
            const ImgData&  img = t->img();

            body << (j ? ' ' : ',') << "{ \"title\": \"" << img.title << "\", \"datetime\": \"" << t->idx().key.dt.hms << "\", \"comment\": \"" << img << "\", \"src\": \"" << img.filename << "\", \"thumb\": \"" << t->prevpath() << "\", \"rating\": " << (img.rating.empty() ? "0" : img.rating.c_str()) << ", \"alt\": [";

            if (t->idx().imgs.size() > 1)
            {
                bool k = true;
                ImgIdx::Imgs::const_iterator  alts = t->idx().imgs.begin();
                while (alts != t->idx().imgs.end()) {
                    const ImgData&  altimg = *alts++;
                    body << (k ? ' ' : ',') << "\"" << altimg.filename << "\"";
                    k = false;
                }
            }
            body << "]}";
            j = false;
        }

        body << "] }";
        std::cout << std::endl;
    }

    body << "], \"count\":" << imgttl << ", \"blocks\":" << payloads_.size() << ", \"date_start\": \"" << datestart << "\", \"date_end\": \"" << dateend << "\"";
    _genJsonStat(body, ttlstats);
    body << " }";
    return std::move(body.str());
}
