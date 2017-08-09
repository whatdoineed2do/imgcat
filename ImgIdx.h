#ident "%W%"

/*  $Id: ImgIdx.h,v 1.1 2011/10/30 17:33:53 ray Exp $
 */

#ifndef IMG_TREE_H
#define IMG_TREE_H

#pragma ident  "@(#)$Id: ImgIdx.h,v 1.1 2011/10/30 17:33:53 ray Exp $"

#include <list>
using namespace  std;

#include "ImgKey.h"
#include "ImgData.h"


// keeps track of all images that are same (jpg/nef etc) are indexed off one
// entry

class ImgIdx
{
  public:
    typedef list<ImgData>  Imgs;
    struct Ent {
	Ent(const ImgKey& key_) : key(key_) { }
	Ent(const Ent& rhs_) : key(rhs_.key), imgs(rhs_.imgs) { }

	ImgKey  key;
	Imgs    imgs;

	static bool sortop(const ImgIdx::Ent& l_, const ImgIdx::Ent& r_)
	{ return l_.key < r_.key; }
    };
    typedef list<ImgIdx::Ent>  Idx;

    typedef Idx::iterator        iterator;
    typedef Idx::const_iterator  const_iterator;

    iterator        begin()        { return _idx.begin(); }
    const_iterator  begin() const  { return _idx.begin(); }

    iterator        end()          { return _idx.end();   }
    const_iterator  end() const    { return _idx.end();   }

    bool  empty() const
    { return _idx.empty(); }
    
    Idx::size_type  size() const
    { return _idx.size(); }


    ImgIdx(const char* id_) : id(id_)  { }

    iterator        find(const ImgKey&)        throw ();
    const_iterator  find(const ImgKey&) const  throw ();

    /* this is the ONLY function that will create the Ent if not present
     */
    Imgs&        operator[](const ImgKey&)        throw ();
    const Imgs&  operator[](const ImgKey&) const  throw (range_error);


    void  sort()  throw();

    const string  id;


  private:
    ImgIdx(const ImgIdx&);
    void operator=(const ImgIdx&);

    ImgIdx::Idx  _idx;
};

#endif
