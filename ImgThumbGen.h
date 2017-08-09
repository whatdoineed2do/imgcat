#ident "%W%"

/*  $Id: ImgThumbGen.h,v 1.1 2011/10/30 17:33:53 ray Exp $
 */

#ifndef IMG_THUMB_GEN_H
#define IMG_THUMB_GEN_H

#pragma ident  "@(#)$Id: ImgThumbGen.h,v 1.1 2011/10/30 17:33:53 ray Exp $"


class ImgThumbGen
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
