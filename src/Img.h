#ident "%W%"

/*  $Id: Img.h,v 1.1 2011/10/30 17:33:52 ray Exp $
 */

#ifndef IMG_H
#define IMG_H

#pragma ident  "@(#)$Id: Img.h,v 1.1 2011/10/30 17:33:52 ray Exp $"


#include "ImgKey.h"
#include "ImgData.h"

class Img
{
  public:
    Img(const ImgKey& k_, const ImgData& d_) throw ()
	: key(k_), data(d_)
    { }
 
    Img(const Img& rhs_)  throw ()
	: key(rhs_.key), data(rhs_.data)
    { }

    const ImgKey   key;
    const ImgData  data;

  private:
    void operator=(const Img&);

};

#endif
