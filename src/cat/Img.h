#ident "%W%"

/*  $Id: Img.h,v 1.1 2011/10/30 17:33:52 ray Exp $
 */

#ifndef IMG_H
#define IMG_H

#pragma ident  "@(#)$Id: Img.h,v 1.1 2011/10/30 17:33:52 ray Exp $"


#include "ImgKey.h"
#include "ImgData.h"

struct Img
{
    Img(const ImgKey& k_, const ImgData& d_) noexcept
	: key(k_), data(d_)
    { }

    void operator=(const Img&)  = delete;
    void operator=(const Img&&) = delete;

    Img(const Img& rhs_)  noexcept
	: key(rhs_.key), data(rhs_.data)
    { }
 
    Img(const Img&& rhs_)  noexcept
	: key(std::move(rhs_.key)), data(std::move(rhs_.data))
    { }

    const ImgKey   key;
    const ImgData  data;
};

#endif
