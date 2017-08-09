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

const ImgIdx::Imgs&  ImgIdx::operator[](const ImgKey& k_) const  throw (range_error)
{
    const_iterator p;
    if ( (p = find(k_)) == end()) {
	throw range_error("no such key");
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


#if 0
void  ImgIdx::push_back(const ImgKey& k_, const ImgData& d_)
{
    iterator  i;
    if ( (i = find(k_)) == end()) {
	// this needs to be better - sort but on what?
	i = _idx.insert(_idx.end(), ImgIdx::Ent(k_));
    }

    i->imgs.push_back(d_);
}
#endif
