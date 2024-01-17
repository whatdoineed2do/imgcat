#ident "%W%"

/*  $Id: ImgIdx.cc,v 1.1 2011/10/30 17:33:53 ray Exp $
 */

#pragma ident  "@(#)$Id: ImgIdx.cc,v 1.1 2011/10/30 17:33:53 ray Exp $"

#include "ImgIdx.h"


ImgIdx::iterator ImgIdx::find(const ImgKey& k_)
{
    iterator i;
    for (i=begin(); i!=end(); ++i)
    {
	if (i->key == k_) {
	    break;
	}
    }
    return i;
}


ImgIdx::const_iterator  ImgIdx::find(const ImgKey& k_) const
{
    const_iterator i;
    for (i=begin(); i!=end(); ++i)
    {
	if (i->key == k_) {
	    break;
	}
    }
    return i;
}


ImgIdx::Imgs&  ImgIdx::operator[](const ImgKey& k_)
{
    iterator p;
    if ( (p = find(k_)) == end()) {
	p = _idx.insert(_idx.end(), ImgIdx::Ent(k_));

    }
    return p->imgs;
}

const ImgIdx::Imgs&  ImgIdx::operator[](const ImgKey& k_) const
{
    const_iterator p;
    if ( (p = find(k_)) == end()) {
	throw std::range_error("no such key");
    }
    return p->imgs;
}


void  ImgIdx::sort()
{
    _idx.sort(ImgIdx::Ent::sortop);
    for (iterator i=begin(); i!=end(); ++i) {
	i->imgs.sort();
    }
}


/* summarise the stats based on what we know at this point .. we can't do it in
 * the operator[] as we dont add anyhting at that point, its just creating the entry
 * ... maybe that needs to change
 */
ImgIdx::Stats  ImgIdx::stats()
{
    ImgIdx::Stats  stats;
    for (const_iterator i=begin(); i!=end(); ++i)
    {
        for (const auto&  k : i->imgs) {
            stats.camera[k.metaimg.camera]++;
            stats.lens[k.metaimg.lens]++;
            stats.focallen[k.metaimg.focallen]++;
        }
    }
    return stats;
}


ImgIdx::Stats&  ImgIdx::Stats::operator+=(const ImgIdx::Stats& rhs_)
{
    std::list<std::pair<ImgIdx::Stats::_Stat*, const ImgIdx::Stats::_Stat*>>  l = { 
	std::make_pair(&camera,   &rhs_.camera),
	std::make_pair(&lens,     &rhs_.lens),
	std::make_pair(&focallen, &rhs_.focallen)
    };
    for (const auto& i : l) {
	auto&  x = *i.first;
	auto&  y = *i.second;

	for (const auto& j : y) {
	    x[j.first] += j.second;
	}
    }
    return *this;
}
