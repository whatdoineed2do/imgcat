#ident "%W%"

/*  $Id: ImgIdx.h,v 1.1 2011/10/30 17:33:53 ray Exp $
 */

#ifndef IMG_IDX_H
#define IMG_IDX_H

#pragma ident  "@(#)$Id: ImgIdx.h,v 1.1 2011/10/30 17:33:53 ray Exp $"

#include <list>
#include <string>
#include <unordered_map>

#include "ImgKey.h"
#include "ImgData.h"


// keeps track of all images that are same (jpg/nef etc) are indexed off one
// entry

class ImgIdx
{
  public:
    typedef std::list<ImgData>  Imgs;
    struct Ent {
	Ent(const ImgKey& key_) : key(key_) { }
	Ent(const Ent& rhs_) : key(rhs_.key), imgs(rhs_.imgs) { }

	Ent(const ImgKey&& key_) : key(std::move(key_)) { }
	Ent(const Ent&& rhs_) : key(std::move(rhs_.key)), imgs(std::move(rhs_.imgs)) { }

	ImgKey  key;
	Imgs    imgs;

	static bool sortop(const ImgIdx::Ent& l_, const ImgIdx::Ent& r_)
	{ return l_.key < r_.key; }
    };
    typedef std::list<ImgIdx::Ent>  Idx;

    struct Stats {
        typedef std::unordered_map<std::string, unsigned>  _Stat;

        _Stat  camera;
        _Stat  lens;
        _Stat  focallen;

	Stats&  operator+=(const ImgIdx::Stats&);
    };

    typedef Idx::iterator        iterator;
    typedef Idx::const_iterator  const_iterator;

    iterator        begin()        { return _idx.begin(); }
    const_iterator  begin() const  { return _idx.begin(); }

    iterator        end()          { return _idx.end();   }
    const_iterator  end() const    { return _idx.end();   }


    typedef Idx::reverse_iterator        reverse_iterator;
    typedef Idx::const_reverse_iterator  const_reverse_iterator;

    reverse_iterator        rbegin()         { return _idx.rbegin(); }
    const_reverse_iterator  crbegin() const  { return _idx.crbegin(); }

    reverse_iterator        rend()           { return _idx.rend();   }
    const_reverse_iterator  crend() const    { return _idx.crend();   }

    auto&        front()        { return _idx.front(); }
    const auto&  front() const  { return _idx.front(); }

    auto&        back()        { return _idx.back(); }
    const auto&  back() const  { return _idx.back(); }


    bool  empty() const
    { return _idx.empty(); }
    
    Idx::size_type  size() const
    { return _idx.size(); }


    ImgIdx(const char* id_) : id(id_)  { }

    ImgIdx(const ImgIdx&) = delete;
    ImgIdx(const ImgIdx&& rhs_) : _idx(std::move(rhs_._idx)), id(std::move(rhs_.id))
    { }

    void operator=(const ImgIdx&)  = delete;
    void operator=(const ImgIdx&&) = delete;

    iterator        find(const ImgKey&)        throw ();
    const_iterator  find(const ImgKey&) const  throw ();

    /* this is the ONLY function that will create the Ent if not present
     */
    Imgs&        operator[](const ImgKey&)        throw ();
    const Imgs&  operator[](const ImgKey&) const  throw (std::range_error);


    void  sort()  throw();
    ImgIdx::Stats  stats();

    const std::string  id;

  private:
    ImgIdx::Idx  _idx;
};

using ImgIdxs = std::list<ImgIdx>;

#endif
