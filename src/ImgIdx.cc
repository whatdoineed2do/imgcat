#ident "%W%"

/*  $Id: ImgIdx.cc,v 1.1 2011/10/30 17:33:53 ray Exp $
 */

#pragma ident  "@(#)$Id: ImgIdx.cc,v 1.1 2011/10/30 17:33:53 ray Exp $"

#include "ImgIdx.h"


ImgIdx::iterator ImgIdx::find(const ImgKey& k_)  throw ()
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


ImgIdx::const_iterator  ImgIdx::find(const ImgKey& k_) const throw ()
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


ImgIdx::Imgs&  ImgIdx::operator[](const ImgKey& k_)  throw ()
{
    iterator p;
    if ( (p = find(k_)) == end()) {
	p = _idx.insert(_idx.end(), ImgIdx::Ent(k_));

    }
    return p->imgs;
}

const ImgIdx::Imgs&  ImgIdx::operator[](const ImgKey& k_) const  throw (std::range_error)
{
    const_iterator p;
    if ( (p = find(k_)) == end()) {
	throw std::range_error("no such key");
    }
    return p->imgs;
}


void  ImgIdx::sort()  throw ()
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
